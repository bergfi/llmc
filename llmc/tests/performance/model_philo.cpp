#include <fstream>
#include <llmc/model.h>
#include <llmc/statespace/listener.h>
#include "libfrugi/include/libfrugi/Settings.h"
#include <llmc/modelcheckers/interface.h>
#include <llmc/modelcheckers/multicoresimple.h>
#include <llmc/modelcheckers/singlecore.h>
#include <llmc/modelcheckers/multicore.h>
#include <llmc/statespace/listener.h>
#include <llmc/storage/interface.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/dtree2.h>
#include <llmc/storage/stdmap.h>
#include <llmc/storage/cchm.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>

using StateSlot = llmc::storage::StorageInterface::StateSlot;

typedef uint64_t MemOffset;

struct StateIdentifier {
    size_t getLength() const {
        return data >> 40;
    }

    template<typename Context>
    void pull(Context* ctx, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, 0, slots, getLength(), isRoot);
    }

    template<typename Context>
    void pull(Context* ctx, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, 0, (StateSlot*)slots, getLength(), isRoot);
    }

    template<typename Context>
    void pullPartial(Context* ctx, size_t offset, size_t length, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset, slots, length, isRoot);
    }

    template<typename Context>
    void pullPartial(Context* ctx, size_t offset, size_t length, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset, (StateSlot*)slots, length, isRoot);
    }

    template<typename Context>
    void push(Context* ctx, StateSlot* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newTransition(ctx, length, slots, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, length, slots).getData();
        }
    }

    template<typename Context>
    void push(Context* ctx, StateSlot* slots, size_t length, std::string const& label) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();

        StateIdentifier labelID = {0};
        if(label.length() >= 8) {
            labelID.init(ctx, (StateSlot*) label.c_str(), label.length() / 4);
        }

        data = mc->newTransition(ctx, length, slots, TransitionInfoUnExpanded::construct(labelID)).getData();
    }

    template<typename Context>
    void push(Context* ctx, void* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newTransition(ctx, length, (StateSlot*)slots, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, length, (StateSlot*)slots).getData();
        }
    }

    template<typename Context>
    void init(Context* ctx, StateSlot* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newState(ctx, 0, length, slots).getState().getData();
        } else {
            data = mc->newSubState(ctx, length, slots).getData();
        }
    }

    template<typename Context>
    void init(Context* ctx, void* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newState(ctx, 0, length, (StateSlot*)slots).getState().getData();
        } else {
            data = mc->newSubState(ctx, length, (StateSlot*)slots).getData();
        }
    }

    template<typename Context>
    size_t appendBytes(Context* ctx, size_t length, const void* slots, bool isRoot = false) {
        size_t len = getLength();
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        auto d = mc->newDelta(len, (StateSlot*)slots, length/4);
        if(isRoot) {
            data = mc->newTransition(ctx, *d, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, data, *d).getData();
        }
        mc->deleteDelta(d);
        return len * 4;
    }

    template<typename Context>
    void modifyBytes(Context* ctx, size_t offset, size_t length, const void* slots, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        auto d = mc->newDelta(offset/4, (StateSlot*)slots, length/4);
        if(isRoot) {
            data = mc->newTransition(ctx, *d, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, data, *d).getData();
        }
        mc->deleteDelta(d);
    }

    template<typename Context>
    void readBytes(Context* ctx, size_t offset, size_t length, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset/4, slots, length/4, isRoot);
    }

    template<typename Context>
    void readBytes(Context* ctx, size_t offset, size_t length, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset/4, (StateSlot*)slots, length/4, isRoot);
    }

    template<typename Context, typename T>
    void readMemory(Context* ctx, size_t offset, T& var, bool isRoot = false) const {
        readBytes(ctx, offset, sizeof(T), &var, isRoot);
    }

    template<typename Context, typename T>
    void writeMemory(Context* ctx, size_t offset, T const& var, bool isRoot = false) {
        modifyBytes(ctx, offset, sizeof(T), &var, isRoot);
    }

    template<typename Context, typename T>
    bool cas(Context* ctx, MemOffset ptr, T& expected, T const& desired) {
        T localCopy;
        readBytes(ctx, ptr, sizeof(T), &localCopy);

        // CAS failed
        if(memcmp(&localCopy, &expected, sizeof(T))) {
            memcpy(&expected, &localCopy, sizeof(T));
            return true;

            // CAS succeeded
        } else {
            modifyBytes(ctx, ptr, sizeof(T), &desired);
            return false;
        }
    }


    uint64_t data;
};

enum class DidWhat {
    NOTHING,
    SOMETHING,
    ENDED
};

struct Philo {
    uint32_t id;
    uint32_t state;
    uint32_t pc;
    uint32_t fork;
};

struct SV {
    uint32_t state;
    uint32_t philos;
    StateIdentifier philo[];
};


class MSQModel: public VModel<llmc::storage::StorageInterface> {
public:
    size_t getNextAll(StateID const& s, Context* ctx) override {
        StateIdentifier id{s.getData()};
        StateSlot svmem[id.getLength()];
        id.pull(ctx, svmem, true);
        SV& sv = *(SV*)svmem;
//        printf("getNextAll %u %u\n", sv.state, sv.procs);
        size_t r = 0;
        StateSlot svmemCopy[id.getLength()];

        for(size_t idx = sv.philos; idx--;) {

            std::stringstream ss;

            memcpy(svmemCopy, svmem, sizeof(svmemCopy));
            SV& svCopy = *(SV*)svmemCopy;

            StateSlot philomem[svCopy.philo[idx].getLength()];
            svCopy.philo[idx].pull(ctx, philomem);
            Philo& philo = *(Philo*)philomem;

            auto doneSomething = getNextPhilo(s, ctx, philo, ss);
            if(doneSomething != DidWhat::NOTHING) {
                svCopy.philo[idx].push(ctx, philomem, svCopy.philo[idx].getLength());
                id.push(ctx, svmemCopy, id.getLength(), ss.str());
                r++;
            }
        }

        if(r == 0) {
            // TODO: check for correct end state
        }

        return r;
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        abort();
    }

    DidWhat getNextPhilo(StateID const& s, Context* ctx, Philo& proc, std::ostream& label) {
        auto pcCurrent = proc.pc;

        label << "[" << proc.id << "@" << pcCurrent << "] ";
//        std::cout << "[" << proc.id << "@" << pcCurrent << "] " << std::endl;

        proc.pc++;
//        printf("PC %u;\n", pcCurrent);
        switch(pcCurrent) {
            case 0:
                return DidWhat::NOTHING;
            default:
                abort();
        }
        return proc.state ? DidWhat::ENDED : DidWhat::SOMETHING;
    }

    StateID getInitial(Context* ctx) override {
        Settings& settings = Settings::global();


        size_t philos = 1;

        if(settings["philos.philos"].asUnsignedValue()) {
            philos = settings["philos.philos"].asUnsignedValue();
        }

        size_t initSize = sizeof(SV) + (philos) * sizeof(StateIdentifier);
        assert((initSize & 3) == 0);

        StateSlot svmem[initSize / 4];
        memset(svmem, 0, initSize);
        SV& sv = *(SV*)svmem;
        sv.state = 3;

        // TODO: init

        StateIdentifier init;
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

        StateIdentifier labelID = tinfo_.get<StateIdentifier>();
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

    auto model = new MSQModel();
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

    __attribute__((always_inline))
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
//    } else if(settings["storage"].asString() == "dtree2") {
//        goPINS<llmc::storage::DTree2Storage<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>,SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(fileName);
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
//    while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
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
