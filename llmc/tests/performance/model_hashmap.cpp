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

struct HashMap {
    uint64_t size;
    uint64_t buckets[];
};

struct EProc {
    MemOffset insertedStringPointer;
    size_t insertedStringLength;
    ModelStateIdentifier toInsertStr;
};

struct DProc {
};

struct Proc {
    uint32_t id;
    uint32_t state;
    uint32_t pc;
    uint32_t type;
    uint64_t result;
    union {
        EProc e;
        DProc d;
    };
};

struct SV {
    uint32_t state;
    uint32_t procs;
    ModelStateIdentifier memory;
    ModelStateIdentifier proc[];
};


class HashMapModel: public VModel<llmc::storage::StorageInterface> {
public:
    size_t getNextAll(StateID const& s, Context* ctx) override {
        ModelStateIdentifier id{s.getData()};
        StateSlot svmem[id.getLength()];
        id.pull(ctx, svmem, true);
        SV& sv = *(SV*)svmem;
//        printf("getNextAll %u %u\n", sv.state, sv.procs);
        size_t r = 0;
        StateSlot svmemCopy[id.getLength()];

        for(size_t idx = sv.procs; idx--;) {

            std::stringstream ss;

            memcpy(svmemCopy, svmem, sizeof(svmemCopy));
            SV& svCopy = *(SV*)svmemCopy;

            StateSlot procmem[svCopy.proc[idx].getLength()];
            svCopy.proc[idx].pull(ctx, procmem);
            Proc& proc = *(Proc*)procmem;

//            printf("getNextProcess %u %u %u\n", idx, svCopy.proc[idx].data, proc.pc);

            auto doneSomething = getNextProcess(s, ctx, svCopy.memory, proc, ss);
            if(doneSomething != DidWhat::NOTHING) {
                svCopy.proc[idx].push(ctx, procmem, svCopy.proc[idx].getLength());
                id.push(ctx, svmemCopy, id.getLength(), ss.str());
                r++;
            }
        }

        if(r == 0) {
            bool allOK = true;
            for(size_t idx = sv.procs; idx--;) {
                uint32_t procState = 0;
                sv.proc[idx].readBytes(ctx, offsetof(Proc, pc), sizeof(Proc::pc), &procState);
                if(procState != 0) {
                    printf("Proc not finished: %zu\n", idx);
                    allOK = false;
                }
            }
            StateSlot memory[sv.memory.getLength()];
            sv.memory.pull(ctx, memory);

            HashMap& hm = *(HashMap*)memory;
            bool found[sv.procs];
            memset(found, 0, sizeof(found));
            size_t foundTotal = 0;
            for(size_t idx = hm.size; idx--;) {
//                printf("idx: %u %zu\n", idx, sv.memory.getLength()); fflush(stdout);
                uint64_t& bucket = hm.buckets[idx];
                if(bucket) {
                    MemOffset p = bucket & 0xFFFFFFFFULL;
                    size_t len = bucket >> 32;
                    char str[len];
                    sv.memory.readBytes(ctx, p, len, str);
                    size_t id = *str - 'A';
                    if(id < sv.procs) {
                        if(found[id]) {
                            printf("Double found %zu\n", id);
                            allOK = false;
                        } else {
                            foundTotal++;
                            found[id] = true;
                        }
                    } else {
                        printf("Found an id that is too high %zu\n", id);
                        allOK = false;
                    }
                }
            }
            if(foundTotal != sv.procs) {
                printf("Not all found\n");
                allOK = false;
            }
            if(allOK) {
//                printf("Valid end state %zx\n", s.getData());
            }
        }

        return r;
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        abort();
    }

    DidWhat getNextProcess(StateID const& s, Context* ctx, ModelStateIdentifier& mem, Proc& proc, std::ostream& label) {
        auto pcCurrent = proc.pc;

        label << "[" << proc.id << "@" << pcCurrent << "] ";
//        std::cout << "[" << proc.id << "@" << pcCurrent << "] " << std::endl;

        proc.pc++;
//        printf("PC %u;\n", pcCurrent);
        switch(pcCurrent) {
            case 0: {
                return DidWhat::NOTHING;

            // Allocate memory for the string and initialise
            } case 1: {
                StateSlot strToInsert[proc.e.toInsertStr.getLength()];
                proc.e.toInsertStr.pull(ctx, strToInsert);
                proc.e.insertedStringPointer = mem.appendBytes(ctx, sizeof(strToInsert), strToInsert);
                proc.e.insertedStringLength = sizeof(strToInsert);
//                printf("init string %s\n", (char*)strToInsert);
                return DidWhat::SOMETHING;

            // Determine index and insert
            } case 2: {

                char str[proc.e.insertedStringLength];
                mem.readBytes(ctx, proc.e.insertedStringPointer, proc.e.insertedStringLength, str);

                // very crude hash
                size_t h = 0;
                char* strCurrent = str;
                char* strEnd = str + proc.e.insertedStringLength;
                while(strCurrent < strEnd) {
                    h ^= *strCurrent++;
                }

                uint64_t hmSize;
                mem.readMemory(ctx, 0, hmSize);

                h = h % hmSize;

                MemOffset hashMapBuckets = sizeof(HashMap); // statically known

                uint64_t expected = 0;
                uint64_t desired = (uint64_t)proc.e.insertedStringPointer | ((uint64_t)proc.e.insertedStringLength << 32);
                mem.cas(ctx, (hashMapBuckets + h * sizeof(*HashMap::buckets)), expected, desired);
                proc.pc = 0;
//                printf("inserted string at (%zu+%zu*%u) %u\n", hashMapBuckets, h, sizeof(*HashMap::buckets), (hashMapBuckets + h * sizeof(*HashMap::buckets)));
                return DidWhat::SOMETHING;
            } default:
                printf("WRONG PC: %u\n", pcCurrent);
                exit(-1);
        }
        return proc.state ? DidWhat::ENDED : DidWhat::SOMETHING;
    }

    StateID getInitial(Context* ctx) override {
        Settings& settings = Settings::global();

        size_t inserts = 1;
        size_t doubleInserts = 0;
        size_t buckets = 64;
        size_t strLengths = 64;

        if(settings["model.hashmap.inserts"].asUnsignedValue()) {
            inserts = settings["model.hashmap.inserts"].asUnsignedValue();
        }
        if(settings["model.hashmap.inserts"].asUnsignedValue()) {
            doubleInserts = settings["model.hashmap.doubles"].asUnsignedValue();
        }
        if(settings["model.hashmap.buckets"].asUnsignedValue()) {
            buckets = settings["model.hashmap.buckets"].asUnsignedValue();
        }

        size_t initSize = sizeof(SV) + (inserts + doubleInserts) * sizeof(ModelStateIdentifier);
        assert((initSize & 3) == 0);

        StateSlot svmem[initSize / 4];
        memset(svmem, 0, initSize);
        SV& sv = *(SV*)svmem;
        sv.state = 3;
        sv.procs = inserts + doubleInserts;
        sv.memory = {0};
        size_t procID = 0;
        for(size_t e = inserts; e--;) {
            Proc eproc = {0};
            eproc.id = procID + 1;
            eproc.state = 1;
            eproc.pc = 1;
            eproc.type = 1;

            char str[strLengths];
            memset(str, 'A' + procID, strLengths);
            str[strLengths-1] = 0;
            eproc.e.toInsertStr.initBytes(ctx, str, strLengths);

            sv.proc[procID].init(ctx, &eproc, sizeof(Proc)/4);
            printf("Uploading enqueue %u %zx\n", eproc.pc, sv.proc[procID].data);
            procID++;
        }
        for(size_t e = doubleInserts; e--;) {
            Proc eproc = {0};
            eproc.id = procID + 1;
            eproc.state = 1;
            eproc.pc = 1;
            eproc.type = 1;

            char str[strLengths];
            memset(str, 'A' + e, strLengths);
            str[strLengths-1] = 0;
            eproc.e.toInsertStr.initBytes(ctx, str, strLengths);

            sv.proc[procID].init(ctx, &eproc, sizeof(Proc)/4);
            printf("Uploading enqueue %u %zx\n", eproc.pc, sv.proc[procID].data);
            procID++;
        }

        StateSlot memory[(sizeof(HashMap) + sizeof(uint64_t) * buckets)/4];
        memset(memory, 0, sizeof(memory));

        HashMap& hm = *(HashMap*)memory;

        hm.size = buckets;

        sv.memory.init(ctx, memory, sizeof(memory)/4);
        printf("Uploading initial memory %zx (%zu)\n", sv.memory.data, sv.memory.getLength());

        ModelStateIdentifier init;
        init.init(ctx, svmem, initSize / 4, true);
        printf("Uploading initial state %zx %u %u length: %zu, first proc: %zu\n", init.data, sv.state, sv.procs, initSize / 4, sv.proc[0].data);
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

    auto model = new HashMapModel();
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
        goMSQ<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>();
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
