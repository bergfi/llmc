#include <fstream>
#include <llmc/model.h>
#include <llmc/statespace/listener.h>
#include "libfrugi/include/libfrugi/Settings.h"
#include <llmc/modelcheckers/interface.h>
#include <llmc/modelcheckers/multicoresimple.h>
#include <llmc/modelcheckers/singlecore.h>
#include <llmc/modelcheckers/multicore_bitbetter.h>
#include <llmc/statespace/listener.h>
#include <llmc/storage/interface.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/dtree2.h>
#include <llmc/storage/stdmap.h>
#include <llmc/storage/cchm.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>
#include "StateIdentifier.h"

using StateSlot = llmc::storage::StorageInterface::StateSlot;
using ModelStateIdentifier = StateIdentifier<StateSlot>;

typedef uint64_t MemOffset;


enum class DidWhat {
    NOTHING,
    SOMETHING,
    ENDED
};

struct Philo {
    uint32_t id;
    uint32_t pc;
    uint32_t state;  // number of times Philo ate
    uint32_t fork;  // 'tis our "left fork" -- our "right fork" is held by the next Philo
    // values ^^^: 0 == "free" ; 1 == "I have it" ; 2 == "Someone else has it"
    static constexpr auto MAX_EATS = 3ul;
};

struct SV {
    uint32_t state;
    uint32_t philos;
    ModelStateIdentifier philo[];
};


class PhiloModel: public VModel<llmc::storage::StorageInterface> {
public:
    size_t getNextAll(StateID const& s, Context* ctx) override {
        ModelStateIdentifier id{s.getData()};
        StateSlot svmem[id.getLength()];
        id.pull(ctx, svmem, true);
        SV& sv = *(SV*)svmem;
//	  printf("getNextAll %u %u\n", sv.state, sv.procs);
        size_t r = 0;
        StateSlot svmemCopy[id.getLength()];

        for(size_t idx = sv.philos; idx--;) {

            std::stringstream ss;

            memcpy(svmemCopy, svmem, sizeof(svmemCopy));
            SV& svCopy = *(SV*)svmemCopy;

            // get (sub-state of) this philo
            auto& thisphilo = svCopy.philo[idx];
            StateSlot philomem[thisphilo.getLength()];
            thisphilo.pull(ctx, philomem);
            Philo& philo = *(Philo*)philomem;

            // get reference to next philo (who holds our right fork)
            const auto idxnext = (idx+1) % svCopy.philos;
            auto& nextphilo = svCopy.philo[idxnext];

            // make philo do something  (good luck with that)
            auto doneSomething = getNextPhilo(ctx, philo, nextphilo, ss);
            //printf("%s\n", ss.str().c_str());
            // if philo did something, update:
            if(doneSomething != DidWhat::NOTHING) {
                // first its own contents (i.e. the sub-state) in the svCopy.philo vector
                thisphilo.push(ctx, philomem, thisphilo.getLength());
                // then the whole (changed) state of the model checker
                id.push(ctx, svmemCopy, id.getLength(), ss.str());
                r++;
            }
        }

        if (r == 0) {
            // Check for correct end state
            bool allSatisfied = true;
            decltype(Philo::state) state = 0;
            auto idx = sv.philos;
            for (; allSatisfied && idx-- ;) {
                sv.philo[idx].readMemory(ctx, offsetof(Philo,state), state);
                allSatisfied &= state == Philo::MAX_EATS;
            }
            if (allSatisfied)
                printf("All philosopher are satisfied\n");
            else
                printf("Philosopher %d is still hungry: it ate %u (out of %u) times\n", idx, state, Philo::MAX_EATS);
        }

        return r;
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        abort();
    }

    /// @brief This implements a naive algorithm to the philosophers problem:
    ///			0. think
    ///			1. try to pick left  fork; failed ? goto 0.
    ///			2. try to pick right fork; failed ? release left fork && goto 0.
    ///			3. eat
    ///			4. release left fork
    ///			5. release right fork
    ///			6. hungry ? goto 0. : die
    /// @param  p: this Philo (fetched from the MC state)
    /// @param np: next Philo (its ModelStateIdentifier)
    /// @return  DidWhat::NOTHING   if 6. above and no longer hungry
    ///	         DidWhat::SOMETHING if 0. to 6. above (except ^^^ )
    ///	         DidWhat::ENDED	 otherwise (error)
    DidWhat getNextPhilo(Context* ctx,
                         Philo& p,
                         ModelStateIdentifier& np,
                         std::ostream& label)
    {
        using Fork = decltype(Philo::fork);
        static auto constexpr OFFSET = offsetof(Philo,fork);
        auto pcCurrent = p.pc;

        label << "[" << p.id << "@" << pcCurrent << "] ";

        p.pc++;
        switch(pcCurrent)
        {
        case 0 : {
            label << "thinking ";
            return DidWhat::SOMETHING;
        }
        case 1 : {
            label << "trying to pick left fork... ";
            Fork expected = 0;
            const Fork desired = 1;
            if (p.fork == 0) {
                label << "succeeded ";
                p.fork = 1;
            } else {
                label << "failed ";
                p.pc = 0;
            }
            return DidWhat::SOMETHING;
        }
        case 2 : {
            label << "trying to pick right fork... ";
            Fork expected = 0;
            const Fork desired = 2;
            auto picked = np.cas(ctx, (MemOffset) OFFSET, expected, desired);
            if (picked) {
                label << "yes! CAS succeeded ";
            } else {
                label << "nope, CAS failed ";
                p.fork = 0;
                label << "so I released left fork ";
                p.pc = 0;
            }
            return DidWhat::SOMETHING;
        }
        case 3 : {
            label << "eating, yum yum ";
            p.state += p.state < Philo::MAX_EATS ? 1 : 0;
            return DidWhat::SOMETHING;
        }
        case 4 : {
            label << "releasing left fork after eating ";
            p.fork = 0;
            return DidWhat::SOMETHING;
        }
        case 5 : {
            label << "releasing right fork after eating ";
            Fork expected = 2;
            const Fork desired = 0;
            auto released = np.cas(ctx, (MemOffset) OFFSET, expected, desired);
            if (released)
                label << "CAS succeeded ";
            else
                label << "CAS failed??? Couldn't release right fork !!! ";
            return DidWhat::SOMETHING;
        }
        case 6 : {
            label << "finished loop ";
            if (p.state < Philo::MAX_EATS) {
                label << "but still hungry ";
                p.pc = 0;
                return DidWhat::SOMETHING;
            } else {
                label << "now I'm ded ";
                p.pc = 6;
                return DidWhat::NOTHING;
            }
        }
        default:
            abort();
            break;
        }
        return DidWhat::ENDED;
    }

    StateID getInitial(Context* ctx) override {
        Settings& settings = Settings::global();

        auto philos = 1ul<<1ul;

        if(settings["philos.philos"].asUnsignedValue()) {
            philos = settings["philos.philos"].asUnsignedValue();
        }

        auto initSize = sizeof(SV) + (philos) * sizeof(ModelStateIdentifier);
        assert( ! (initSize & 3));

        StateSlot svmem[initSize / 4];
        memset(svmem, 0, initSize);
        SV& sv = *(SV*)svmem;
        sv.state = 3;
        sv.philos = philos;
        for (auto i=philos; i-- ;) {
            Philo p;
            p.id = i;
            p.pc = p.state = p.fork = 0;  // shorturl.at/fryDP
            sv.philo[i].init(ctx, &p, sizeof(Philo)/4);
            printf("Uploading philosopher #%d: %zx\n", i, sv.philo[i].data);
        }

        ModelStateIdentifier init;
        init.init(ctx, svmem, initSize / 4, true);
        printf("Uploading initial state %zx %u %u\n", init.data, sv.state, sv.philos);
        return init.data;
    }

    llmc::statespace::Type* getStateVectorType() override {
        return nullptr;
    }

    void init(Context* ctx) override {
    }

    TransitionInfo getTransitionInfo(VContext<llmc::storage::StorageInterface>* ctx,
                                     TransitionInfoUnExpanded const& tinfo_) const override {

        ModelStateIdentifier labelID = tinfo_.get<ModelStateIdentifier>();
        StateSlot str[labelID.getLength()];
        labelID.pull(ctx, str);


        return TransitionInfo(std::string((char*)str, labelID.getLength()*4));
    }

    size_t getStateLength() const override {
        return 0;
    }
};

//template<typename Storage, template <typename, typename,template<typename,typename> typename> typename ModelChecker>
template<typename Storage, template <typename, typename,template<typename,typename> typename> typename ModelChecker>
void goMSQ() {
    Settings& settings = Settings::global();
    using MC = ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>;

    std::ofstream f;
    f.open("out.dot", std::fstream::trunc);

    auto model = new PhiloModel();
    llmc::statespace::DotPrinter<MC, VModel<llmc::storage::StorageInterface>
    > printer(f);
    MC mc(model, printer);
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
}

template<typename T>
struct HashCompareMurmur {
    static constexpr uint64_t hashForZero = 0x7208f7fa198a2d81ULL;
    static constexpr uint64_t seedForZero = 0xc6a4a7935bd1e995ULL*8ull;

    __attribute__((always_inline))  // he he
    bool equal( const T& j, const T& k ) const {
        return j == k;
    }

    __attribute__((always_inline))
    size_t hash( const T& k ) const {
        return MurmurHash64(&k, sizeof(T), seedForZero);
    }
};

template<template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goSelectStorage() {
    Settings& settings = Settings::global();

    if(settings["storage"].asString() == "stdmap") {
        goMSQ<llmc::storage::StdMap, ModelChecker>();
    } else if(settings["storage"].asString() == "cchm") {
        goMSQ<llmc::storage::cchm, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_stdmap") {
        goMSQ<llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_cchm") {
        goMSQ<llmc::storage::TreeDBSStorage<llmc::storage::cchm>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbsmod") {
        goMSQ<llmc::storage::TreeDBSStorageModified, ModelChecker>();
    } else if(settings["storage"].asString() == "dtree") {
        goMSQ<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>();
//	} else if(settings["storage"].asString() == "dtree2") {
//		goPINS<llmc::storage::DTree2Storage<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>,SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(fileName);
    } else {
        std::cout << "Wrong or no storage selected: " << settings["storage"].asString() << std::endl;
    }
}

void go() {
    Settings& settings = Settings::global();

    if(settings["mc"].asString() == "multicore_simple") {
        goSelectStorage<MultiCoreModelCheckerSimple>();
    } else if(settings["mc"].asString() == "multicore_bitbetter") {
        goSelectStorage<MultiCoreModelChecker>();
    } else if(settings["mc"].asString() == "singlecore_simple") {
        goSelectStorage<SingleCoreModelChecker>();
    } else {
        std::cout << "Wrong or no model checker selected: " << settings["mc"].asString() << std::endl;
    }
}

int main(int argc, char** argv) {

    Settings& settings = Settings::global();
    int c = 0;
    while ((c = getopt(argc, argv, "m:s:t:-:")) != -1) {
//	while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
        switch(c) {
            case 't':
                if(optarg) {
                    settings["threads"] = std::stoi(optarg);
                }
                break;
            case 'm':
                if(optarg) {
                    settings["mc"] = std::string(optarg);
                }
                break;
            case 's':
                if(optarg) {
                    settings["storage"] = std::string(optarg);
                }
                break;
            case '-':
                settings.insertKeyValue(optarg);
        }
    }

    go();

}
