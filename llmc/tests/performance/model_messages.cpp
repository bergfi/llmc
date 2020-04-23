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

typedef uint64_t MemOffset;

enum class DidWhat {
    NOTHING,
    SOMETHING,
    ENDED
};

struct Node;

struct MQueue {
    uint64_t messages;
    MemOffset queue[];
};

struct Proc {
    uint64_t data;
    NodePtr tail;
    NodePtr next;
    NodePtr tail2;
    MemOffset node;
    uint32_t i = 0;
};

struct DProc {
    uint64_t data;
    NodePtr head;
    NodePtr head2;
    NodePtr tail;
    NodePtr next;
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
    StateIdentifier threadResults;
    StateIdentifier memory;
    StateIdentifier proc[];
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
                uint32_t procState = 0;
                sv.proc[idx].readBytes(ctx, offsetof(Proc, state), sizeof(Proc::state), &procState);
                allOK = (procState != 0) ? false : allOK;
            }
            StateSlot memory[sv.memory.getLength()];
            sv.memory.pull(ctx, memory);
            MSQ* msq = (MSQ*)memory;
            Node* n1 = (Node*)(((char*)memory) + sizeof(*msq));
            Node* n2 = (Node*)(((char*)memory) + sizeof(*msq) + sizeof(Node));
            Node* n3 = (Node*)(((char*)memory) + sizeof(*msq) + sizeof(Node) + sizeof(Node));
//            printf("msq.Head: {%zu, %zu}\n", msq->Head.ptr, msq->Head.count);
//            printf("msq.Tail: {%zu, %zu}\n", msq->Tail.ptr, msq->Tail.count);
//            printf("n1: {%zu, %zu, %zu} @ %zu\n", n1->next.ptr, n1->next.count, n1->data, (char*)n1 - (char*)memory);
//            printf("n2: {%zu, %zu, %zu} @ %zu\n", n2->next.ptr, n2->next.count, n2->data, (char*)n2 - (char*)memory);
//            printf("n3: {%zu, %zu, %zu} @ %zu\n", n3->next.ptr, n3->next.count, n3->data, (char*)n3 - (char*)memory);
            size_t isUsed[64] = {0};
//            D: 0 0 0
//            D: 10 0 0 x3
//            D: 11 0 0 x3
//            D: 12 0 0 x3
//            D: 10 11 0 x6
//            D: 10 12 0 x6
//            D: 11 12 0 x6
//            D: 10 11 12

            size_t enqueues = 0;
            size_t dequeues = 0;

            for(size_t idx = sv.procs; idx--;) {
                StateSlot procmem[sv.proc[idx].getLength()];
                sv.proc[idx].pull(ctx, procmem);
                Proc& proc = *(Proc*)procmem;

                // If this is a dequeue
                if(proc.type == 2) {
                    ++dequeues;

                    // If the dequeue returned true, indicated it dequeued something
                    if(proc.result) {

                        // If that something is valid
                        if(proc.result) {

                            // Check that it is dequeued only once
                            if(isUsed[proc.result]) {
                                printf("Dequeued twice: %zu\n", proc.result);
                                allOK = false;
                            }
                            isUsed[proc.result] = proc.id;
                        } else {
                            printf("Dequeued 0:\n");
                            allOK = false;
                        }
                    }
                } else {
                    ++enqueues;
                }
            }

            if(enqueues <= dequeues && msq->Head.ptr != msq->Tail.ptr) {
                printf("Head and Tail are not equal: %zu != %zu\n", msq->Head.ptr, msq->Tail.ptr);
                allOK = false;
            }

            if(!allOK) {
                printf("Invalid end state: %zx\n", s.getData());
            } else {
                printf("Valid end state: %zx\n", s.getData());

            }
        }

        return r;
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        abort();
    }

    DidWhat getNextProcess(StateID const& s, Context* ctx, StateIdentifier& threadResults, StateIdentifier& mem, Proc& proc, std::ostream& label) {
        auto pcCurrent = proc.pc;

        label << "[" << proc.id << "@" << pcCurrent << "] ";
//        std::cout << "[" << proc.id << "@" << pcCurrent << "] " << std::endl;

        proc.pc++;
//        printf("PC %u;\n", pcCurrent);
        switch(pcCurrent) {
            case 0:
                return DidWhat::NOTHING;
            case 1: {
                Node n = {0};
                proc.e.node = mem.appendBytes(ctx, sizeof(n), &n);
//                proc.e.node = sizeof(MSQ) + (proc.id) * sizeof(Node);
                label << "node = newNode() = " << proc.e.node << "   ";
                break;
            }
            case 2: {
                Node* node = (Node*) proc.e.node;
                mem.modifyBytes(ctx, (MemOffset) &node->data, sizeof(proc.e.data), &proc.e.data);
                label << "node->data = data = " << proc.e.data << "   " << std::endl;
                break;
            }
            case 3: {
                Node* node = (Node*) proc.e.node;
                NodePtr desired = {0};
                mem.modifyBytes(ctx, (MemOffset) &node->next, sizeof(desired), &desired);
                label << ("node->next = NodePtr();   ");
                break;
            }

                // WHILE
            case 4: {
                MSQ* msq = nullptr;
                mem.readBytes(ctx, (MemOffset) &msq->Tail, sizeof(proc.e.tail), &proc.e.tail);
                label << "tail = mem[" << ((MemOffset) &msq->Tail) << "] = " << "{" << proc.e.tail.ptr << ", "
                      << proc.e.tail.count << "}   ";
                break;
            }
            case 5: {
                mem.readBytes(ctx, proc.e.tail.ptr, sizeof(proc.e.next), &proc.e.next);
                label << "next = mem[" << proc.e.tail.ptr << "] = " << "{" << proc.e.next.ptr << ", "
                      << proc.e.next.count << "}   ";
                break;
            }
            case 6: {
                MSQ* msq = nullptr;
                mem.readBytes(ctx, (MemOffset) &msq->Tail, sizeof(proc.e.tail2), &proc.e.tail2);
                label << "tail2 = mem[" << ((MemOffset) &msq->Tail) << "] = " << "{" << proc.e.tail2.ptr << ", "
                      << proc.e.tail2.count << "}   ";
                break;
            }

                // IF tail.ptr==tail2.ptr && tail.count==tail2.count
            case 7:
                proc.pc = (proc.e.tail.ptr == proc.e.tail2.ptr && proc.e.tail.count == proc.e.tail2.count)
                          ? 8
                          : 4;
                label << "if(tail.ptr==tail2.ptr && tail.count==tail2.count) -> " << proc.pc << "   ";
                break;

                // IF tail.ptr==tail2.ptr && tail.count==tail2.count - TRUE CASE
                // IF !next.ptr
            case 8:
                proc.pc = (!proc.e.next.ptr)
                          ? 9
                          : 10;
                label << "if(!next.ptr) -> " << proc.pc << "   ";
                break;

                // IF !next.ptr - TRUE CASE
            case 9: {
                NodePtr desired{proc.e.node, proc.e.next.count + 1};
                Node* ptr = (Node*) proc.e.tail.ptr;
                if(mem.cas(ctx, (MemOffset) &ptr->next, proc.e.next, desired)) {
                    proc.pc = 4;
                    label << "CAS FAILED, proc.e.next: {" << proc.e.next.ptr << ", " << proc.e.next.count << "}   ";
                } else {
                    proc.pc = 11;
                    label << "CAS SUCCEEDED, mem[proc.e.tail.ptr(" << proc.e.tail.ptr << ")] = {" << proc.e.next.ptr
                          << ", " << proc.e.next.count << "}   ";
                }
                break;

                // IF !next.ptr - FALSE CASE
            }
            case 10: {
                MSQ* msq = nullptr;
                NodePtr desired{proc.e.next.ptr, proc.e.tail.count + 1};
                if(mem.cas(ctx, (MemOffset) &msq->Tail, proc.e.tail, desired)) {
                    label << "CAS FAILED, proc.e.tail: {" << proc.e.tail.ptr << ", " << proc.e.tail.count << "}   ";
                } else {
                    label << "CAS SUCCEEDED, mem[&msq->Tail(" << (MemOffset) &msq->Tail << ")] = {" << desired.ptr
                          << ", " << desired.count << "}   ";
                }
                proc.pc = 4;
                break;
            }
            case 11: {
                MSQ* msq = nullptr;
                NodePtr desired{proc.e.node, proc.e.tail.count + 1};
                if(mem.cas(ctx, (MemOffset) &msq->Tail, proc.e.tail, desired)) {
                    label << "CAS FAILED, proc.e.tail:     {" << proc.e.tail.ptr << ", " << proc.e.tail.count << "}  ";
                } else {
                    label << "CAS SUCCEEDED, mem[&msq->Tail(" << (MemOffset) &msq->Tail << ")] = {" << desired.ptr
                          << ", " << desired.count << "}  ";

                }
                proc = {0};
                proc.pc = 0;
//                printf("CAS(Tail, tail, NodePtr(next.ptr, tail.count + 1)\n");
                break;
            }

                // DEQUEUE
            case 1001: {
                MSQ* msq = nullptr;
                mem.readMemory(ctx, (MemOffset) &msq->Head, proc.d.head);
                label << "head = mem[" << ((MemOffset) &msq->Head) << "] = " << "{" << proc.d.head.ptr << ", " << proc.d.head.count << "}   ";
                break;
            } case 1002: {
                MSQ* msq = nullptr;
                mem.readMemory(ctx, (MemOffset) &msq->Tail, proc.d.tail);
                label << "tail = mem[" << ((MemOffset) &msq->Tail) << "] = " << "{" << proc.d.tail.ptr << ", " << proc.d.tail.count << "}   ";
                break;
            } case 1003: {
                mem.readMemory(ctx, proc.d.head.ptr, proc.d.next);
                label << "next = mem[" << proc.d.head.ptr << "] = " << "{" << proc.d.next.ptr << ", "
                      << proc.d.next.count << "}   ";
                break;
            } case 1004: {
                MSQ* msq = nullptr;
                mem.readMemory(ctx, (MemOffset) &msq->Head, proc.d.head2);
                label << "head2 = mem[" << ((MemOffset) &msq->Head) << "] = " << "{" << proc.d.head2.ptr << ", " << proc.d.head2.count << "}   ";
                break;
            } case 1005: {
                proc.pc = (proc.d.head.ptr == proc.d.head2.ptr && proc.d.head.count == proc.d.head2.count)
                          ? 1006
                          : 1001
                        ;
                label << "if(head.ptr == head2.ptr && head.count == head2.count)   ";
                break;

                // IF head.ptr==head2.ptr && head.count==head2.count - TRUE CASE
            } case 1006: {
                proc.pc = (proc.d.head.ptr == proc.d.tail.ptr)
                          ? 1007
                          : 1008
                        ;
                label << "if(head.ptr == tail.ptr)   ";
                break;

                // IF proc.d.head.ptr == proc.d.tail.ptr - TRUE CASE
            } case 1007: {
                proc.pc = (!proc.d.next.ptr)
                          ? 1009
                          : 1010
                        ;
                label << "if(!next.ptr)   ";
                break;

                // IF proc.d.head.ptr == proc.d.tail.ptr - FALSE CASE
            } case 1008: {
                Node* node = (Node*)proc.d.next.ptr;
                mem.readMemory(ctx, (MemOffset) &node->data, proc.d.data);
                proc.pc = 1011;
                label << "data = node->data = " << proc.d.data << "   ";
                break;
            } case 1009: {
                proc = {0};
                label << "Return nothing   ";
                break;
            } case 1010: {
                MSQ* msq = nullptr;
                NodePtr desired{proc.d.next.ptr, proc.d.tail.count + 1};
                if(mem.cas(ctx, (MemOffset) &msq->Tail, proc.d.tail, desired)) {
                    label << "CAS FAILED, proc.d.tail: {" << proc.d.tail.ptr << ", " << proc.d.tail.count << "}   ";
                } else {
                    label << "CAS SUCCEEDED, mem[&msq->Tail(" << (MemOffset) &msq->Tail << ")] = {" << desired.ptr << ", " << desired.count << "}   ";
                }
                proc.pc = 1001;
                break;
            } case 1011: {
                MSQ* msq = nullptr;
                NodePtr desired{proc.d.next.ptr, proc.d.head.count + 1};
                if(mem.cas(ctx, (MemOffset) &msq->Head, proc.d.head, desired)) {
                    proc.pc = 1001;
                    label << "CAS FAILED, proc.d.tail: {" << proc.d.tail.ptr << ", " << proc.d.tail.count << "}   ";
                } else {
                    label << "CAS SUCCEEDED, mem[&msq->Tail(" << (MemOffset) &msq->Tail << ")] = {" << desired.ptr
                          << ", " << desired.count << "}   ";
                }
                break;
            } case 1012: {
                size_t r = proc.d.data;
//                memset(&proc, 0, sizeof(proc));
                proc = {0};
                proc.result = r;
                label << "Return " << r << "   ";
                break;
            } default:
                abort();
        }
        return proc.state ? DidWhat::ENDED : DidWhat::SOMETHING;
    }

    StateID getInitial(Context* ctx) override {
        Settings& settings = Settings::global();


        size_t enqueues = 1;
        size_t dequeues = 2;

        if(settings["msq.enqueues"].asUnsignedValue()) {
            enqueues = settings["msq.enqueues"].asUnsignedValue();
        }
        if(settings["msq.dequeues"].asUnsignedValue()) {
            dequeues = settings["msq.dequeues"].asUnsignedValue();
        }

        size_t initSize = sizeof(SV) + (enqueues + dequeues) * sizeof(StateIdentifier);
        assert((initSize & 3) == 0);

        StateSlot svmem[initSize / 4];
        memset(svmem, 0, initSize);
        SV& sv = *(SV*)svmem;
        sv.state = 3;
        sv.procs = enqueues + dequeues;
        sv.memory = {0};
        sv.threadResults = {0};
        size_t procID = 0;
        for(size_t e = enqueues; e--;) {
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
        for(size_t d = dequeues; d--;) {
            Proc eproc = {0};
            eproc.id = procID + 1;
            eproc.state = 1;
            eproc.pc = 1001;
            eproc.type = 2;
            sv.proc[procID].init(ctx, &eproc, sizeof(Proc)/4);
            printf("Uploading dequeue %zx\n", sv.proc[procID].data);
            procID++;
        }

        StateSlot memory[(sizeof(MSQ) + sizeof(Node)*(1))/4];
        memset(memory, 0, sizeof(memory));

        MSQ& msq = *(MSQ*)memory;

        MemOffset memStart = sizeof(msq);

        msq.Head = {memStart, 0};
        msq.Tail = {memStart, 0};

        sv.memory.init(ctx, memory, sizeof(memory)/4);
        printf("Uploading initial memory %zx (%zu)\n", sv.memory.data, sv.memory.getLength());

        StateIdentifier init;
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
