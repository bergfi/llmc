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


struct Node {
    MemOffset next;
    uint64_t data;
    char name[32];
};

struct EProc {
    uint64_t data;
    MemOffset nodePointer;
    MemOffset nodePointerChange;
    MemOffset toInsertNodePointer;
    Node node;
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
    ModelStateIdentifier threadResults;
    ModelStateIdentifier memory;
    ModelStateIdentifier proc[];
};

struct SortedList {
    MemOffset Head;
};

class SortedListModel: public VModel<llmc::storage::StorageInterface> {
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

            auto doneSomething = getNextProcess(s, ctx, svCopy.threadResults, svCopy.memory, proc, ss);
            if(doneSomething != DidWhat::NOTHING) {
                svCopy.proc[idx].push(ctx, procmem, svCopy.proc[idx].getLength());
                id.push(ctx, svmemCopy, id.getLength(), ss.str());
                r++;
            }
        }

        if(r == 0) {

            bool allOK = true;

            for(size_t idx = sv.procs; idx--;) {

                std::stringstream ss;

                StateSlot procmem[sv.proc[idx].getLength()];
                sv.proc[idx].pull(ctx, procmem);
                Proc& proc = *(Proc*)procmem;

                if(proc.pc != 0) {
                    allOK = false;
                    printf("Proc idx %zu still going\n", idx);
                }

            }

            MemOffset currentNode;
            sv.memory.readMemory(ctx, 4, currentNode);

            uint64_t currentData = 10;

            size_t nr = 0;

            while(currentNode) {
                Node node;
                sv.memory.readMemory(ctx, currentNode, node);
                if(currentData > node.data) {
                    allOK = false;
                    printf("NOT THE RIGHT ORDER\n");
                } else if(currentData + 1 == node.data) {
                    currentData = node.data;
                } else {
                    allOK = false;
                    printf("NOT THE EXPECTED ONE\n");
                }
                nr++;
                currentNode = node.next;
            }

            if(nr != sv.procs) {
                allOK = false;
                printf("NOT ALL WERE ADDED\n");
            }

            if(!allOK) {
                printf("Invalid end state: %zx\n", s.getData());
            } else {
//                printf("Valid end state: %zx\n", s.getData());

            }
        }

        return r;
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        abort();
    }

    DidWhat getNextProcess(StateID const& s, Context* ctx, ModelStateIdentifier& threadResults, ModelStateIdentifier& mem, Proc& proc, std::ostream& label) {
        auto pcCurrent = proc.pc;

        label << "[" << proc.id << "@" << pcCurrent << "] ";
//        std::cout << "[" << proc.id << "@" << pcCurrent << "] " << std::endl;

        //
        // toInsertNodePointer -> [ next data ]
        //                            |
        //                            |
        //    /---nodePointerChange   |
        //   |                /-------/
        //   |               /------proc.e.nodePointer
        //   v              v
        // [ next data ]    [ next data ]
        //      \---------->



        proc.pc++;
//        printf("PC %u;\n", pcCurrent);
        switch(pcCurrent) {
            case 0:
                return DidWhat::NOTHING;

            // Allocate node
            case 1: {
                proc.e.node.data = proc.e.data;
                proc.e.node.next = 0;
                memcpy(proc.e.node.name, "Process ", 8);
                proc.e.node.name[8] = '0' + proc.id;
                proc.e.node.name[9] = '\0';
                proc.e.toInsertNodePointer = mem.appendBytes(ctx, sizeof(Node), &proc.e.node);
                label << "allocate node " << proc.e.toInsertNodePointer << "   ";
                break;
            }

            // Read head
            case 2: {
                proc.e.nodePointerChange = 4;
                mem.readMemory(ctx, proc.e.nodePointerChange, proc.e.nodePointer);
                label << "read head " << proc.e.nodePointer << "   ";
                break;
            }

            // Read node and compare
            case 3: {
                if(proc.e.nodePointer) {
                    mem.readMemory(ctx, proc.e.nodePointer, proc.e.node);
                    if(proc.e.node.data < proc.e.data) {
                        label << "[" << proc.id << "] go to next node (" << proc.e.nodePointer << ") " << proc.e.node.data << " < " << proc.e.data << "   ";
                        proc.e.nodePointerChange = proc.e.nodePointer;
                        proc.e.nodePointer = proc.e.node.next;
                        proc.pc = 3;
                    }
                }
                break;
            }

            // Update to insert node
            case 4: {
                label << "updating next node of node at " << proc.e.toInsertNodePointer << " to " << proc.e.nodePointer << "   ";
                mem.writeMemory(ctx, proc.e.toInsertNodePointer, proc.e.nodePointer);
                proc.e.node.next = proc.e.nodePointer;
                break;
            }

            // Attempt insert node
            case 5: {
                auto expected = proc.e.nodePointer;
                if(!mem.cas(ctx, proc.e.nodePointerChange, proc.e.nodePointer, proc.e.toInsertNodePointer)) {
                    label << "failed to insert node at " << proc.e.nodePointerChange << ", expected " << expected << ", read " << proc.e.nodePointer << "   ";
                    proc.pc = 3;

                    uint64_t real;
                    mem.readMemory(ctx, proc.e.nodePointerChange, real);
                    label << ", checked real: " << real;

                } else {
                    label << "inserted node (" << proc.e.nodePointer << ", " << proc.e.node.data << ") at " << proc.e.nodePointerChange << "   ";
                    proc.pc = 0;
                }
                break;
            } default:
                abort();
        }
        return proc.state ? DidWhat::ENDED : DidWhat::SOMETHING;
    }

    StateID getInitial(Context* ctx) override {
        Settings& settings = Settings::global();


        size_t inserts = 1;

        if(settings["sortedlist.inserts"].asUnsignedValue()) {
            inserts = settings["sortedlist.inserts"].asUnsignedValue();
        }

        size_t initSize = sizeof(SV) + (inserts) * sizeof(ModelStateIdentifier);
        assert((initSize & 3) == 0);

        StateSlot svmem[initSize / 4];
        memset(svmem, 0, initSize);
        SV& sv = *(SV*)svmem;
        sv.state = 3;
        sv.procs = inserts;
        sv.memory = {0};
        sv.threadResults = {0};
        size_t procID = 0;
        for(size_t e = inserts; e--;) {
            Proc eproc = {0};
            eproc.id = procID + 1;
            eproc.state = 1;
            eproc.pc = 1;
            eproc.type = 1;
            eproc.e.data = 10 + eproc.id;
            sv.proc[procID].init(ctx, &eproc, sizeof(Proc)/4);
            printf("Uploading enqueue %zx\n", sv.proc[procID].data);
            procID++;
        }

        StateSlot memory[4 + (sizeof(SortedList))/4];
        memset(memory, 0, sizeof(memory));

        SortedList& sl = *(SortedList*)&memory[1];

        MemOffset memStart = sizeof(SortedList)+4;

        sl.Head = 0;

        sv.memory.init(ctx, memory, sizeof(memory)/4);
        printf("Uploading initial memory %zx (%zu)\n", sv.memory.data, sv.memory.getLength());

        ModelStateIdentifier init;
        init.init(ctx, svmem, initSize / 4, true);
        printf("Uploading initial state %zx %u %u length: %zu\n", init.data, sv.state, sv.procs, initSize / 4);
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
void go() {
    Settings& settings = Settings::global();
    using MC = ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>;

    std::ofstream f;
    f.open("out.dot", std::fstream::trunc);

    auto model = new SortedListModel();
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
        go<llmc::storage::StdMap, ModelChecker>();
    } else if(settings["storage"].asString() == "cchm") {
        go<llmc::storage::cchm, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_stdmap") {
        go<llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_cchm") {
        go<llmc::storage::TreeDBSStorage<llmc::storage::cchm>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbsmod") {
        go<llmc::storage::TreeDBSStorageModified, ModelChecker>();
    } else if(settings["storage"].asString() == "dtree") {
        go<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>();
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
