
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

struct VoidStream {
    template<typename T>
    friend VoidStream& operator<<(VoidStream& t, T const& anything) {
        return t;
    }
};

enum class DidWhat {
	NOTHING,
	SOMETHING,
	ENDED
};

struct Proc {
//	uint32_t id;
	uint32_t max;
	uint32_t pc;
	uint32_t j;
	//static uint32_t MAGIC_MAX;  // must be assigned length(SV::proc[])+1
	//static constexpr decltype(pc) PC_DONE = 0x101010;
};

//uint32_t Proc::MAGIC_MAX;

struct SV {
    uint32_t choosing[4];
    uint32_t number[4];
    Proc proc[4];
};


class ProcModel: public VModel<llmc::storage::StorageInterface>
{
public:
	size_t getNextAll(StateID const& s, Context* ctx) override {
		StateIdentifier id{s.getData()};
		StateSlot svmem[id.getLength()];
		id.pull(ctx, svmem, true);
		SV& sv = *(SV*)svmem;
		size_t r = 0;
		StateSlot svmemCopy[id.getLength()];

		for(size_t idx = 4; idx--;) {

			std::stringstream ss;

			memcpy(svmemCopy, svmem, sizeof(svmemCopy));
			SV& svCopy = *(SV*)svmemCopy;

			Proc& proc = svCopy.proc[idx];

			// make proc do something
            VoidStream v;
			auto doneSomething = getNextProc(ctx, svCopy, 4, proc, idx, v);
			// if did something, update:
			if(doneSomething != DidWhat::NOTHING) {

				// then the whole (changed) state of the model checker
				id.push(ctx, svmemCopy, id.getLength(), true);
				r++;
			}
		}

//		if (r == 0) {
//            std::cout << "------ end state? " << s.getData() << std::endl;
//			// Check for correct end state
//			decltype(Proc::pc) thisProcPC = 0;
//			bool allDone = true;
//			for (size_t idx = 0; idx < sv.procs; ++idx) {
//                auto& thisproc = sv.proc[idx];
//                StateSlot procmem[thisproc.getLength()];
//                thisproc.pull(ctx, procmem);
//                Proc& proc = *(Proc*)procmem;
//			    std::cout << "proc " << idx << ": " << std::endl;
//			    std::cout << "  id:  " << proc.id << std::endl;
//			    std::cout << "  pc:  " << proc.pc << std::endl;
//			    std::cout << "  j:   " << proc.j << std::endl;
//			    std::cout << "  max: " << proc.max << std::endl;
//			}
//			uint32_t choosing[sv.procs];
//			uint32_t number[sv.procs];
//			sv.choosing.readBytes(ctx, 0, sizeof(choosing), choosing);
//			sv.number.readBytes(ctx, 0, sizeof(number), number);
//			std::cout << "choosing:" ;
//            for (size_t idx = 0; idx < sv.procs; ++idx) {
//                std::cout << " " << choosing[idx];
//            }
//            std::cout << std::endl;
//            std::cout << "number:" ;
//            for (size_t idx = 0; idx < sv.procs; ++idx) {
//                std::cout << " " << number[idx];
//            }
//            std::cout << std::endl;
//		}

//        std::cout << "-> " << std::hex << s.getData() << std::dec << "(" << s.getData() << ") ("  << (s.getData()&0xFFFFFFFFFFULL) << ") -> ";
//		if(r == 0) {
//		    std::cout << "end state";
//		}
//
//        std::cout << std::endl;
//        // Check for correct end state
//        decltype(Proc::pc) thisProcPC = 0;
//        bool allDone = true;
//        {
//            size_t idx = 0;
//            auto& thisproc = sv.proc[idx];
//            StateSlot procmem[thisproc.getLength()];
//            thisproc.pull(ctx, procmem);
//            Proc& proc = *(Proc*)procmem;
//            std::cout << "{P_" << idx << "|" << proc.pc << "}|";
//            std::cout << "{P_" << idx << "|" << proc.id << "}|";
//            std::cout << "{P_" << idx << "|" << proc.j << "}|";
//            std::cout << "{P_" << idx << "|" << proc.max << "}|";
//        }
//        for (size_t idx = 0; idx < sv.procs; ++idx) {
//            std::cout << "{cho|" << choosing[idx] << "}|";
//        }
//        for (size_t idx = 0; idx < sv.procs; ++idx) {
//            std::cout << "{num|" << number[idx] << "}|";
//        }
//        std::cout << "{_nr|2}|";
//        {
//            size_t idx = 1;
//            auto& thisproc = sv.proc[idx];
//            StateSlot procmem[thisproc.getLength()];
//            thisproc.pull(ctx, procmem);
//            Proc& proc = *(Proc*)procmem;
//            std::cout << "{P_" << idx << "|" << proc.pc << "}|";
//            std::cout << "{P_" << idx << "|" << proc.id << "}|";
//            std::cout << "{P_" << idx << "|" << proc.j << "}|";
//            std::cout << "{P_" << idx << "|" << proc.max << "}";
//        }
//        std::cout << std::endl;

		return r;
	}

	size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
		abort();
	}

	/**
	 * @brief This implements a proctype of the bakery.6.prom SPINS model
	 *        Lamport's bakery algorithm is a mutual-exclusion algorithm
	 *        for several processes (clients) trying to gain access to
	 *        some critical section (the baker's counter)
	 * 
	 *  Promela code of proctype P_0:
	 *   1  active proctype P_0() { 
	 *   2    byte j=0;
	 *   3    byte max=0;
	 *   4    NCS:
	 *   5      if
	 *   6      ::  d_step {
	 *   7                  choosing[0] = 1; j = 0; max = 0;}
	 *   8          goto choose; 
	 *   9      fi;
	 *  10    choose:
	 *  11      if
	 *  12      ::  d_step {j<4 && number[j]>max;
	 *  13                  max = number[j]; j = j+1;}
	 *  14          goto choose; 
	 *  15      ::  d_step {j<4 && number[j]<=max;
	 *  16                  j = j+1;}
	 *  17          goto choose; 
	 *  18      ::  d_step {j==4 && max<5;
	 *  19                  number[0] = max+1; j = 0; choosing[0] = 0;}
	 *  20          goto for_loop; 
	 *  21      fi;
	 *  22    for_loop:
	 *  23      if
	 *  24      ::  j<4 && choosing[j]==0;
	 *  25          goto wait; 
	 *  26      ::  j==4;
	 *  27          goto CS; 
	 *  28      fi;
	 *  29    wait:
	 *  30      if
	 *  31      ::  d_step {number[j]==0 || (number[j]<number[0]) || (number[j]==number[0] && 0<=j);
	 *  32          j = j+1;}
	 *  33          goto for_loop; 
	 *  34      fi;
	 *  35    CS:
	 *  36      if
	 *  37      ::  number[0] = 0;
	 *  38          goto NCS; 
	 *  39      fi;
	 *  40  }
	 * 
	 *  In the original Promela file there are 4 proctypes: P_0--P_3.
	 *  These vary on the constant indices used on the arrays number[] and choosing[].
	 *  The following are all the differences between the proctypes P_0 and P_1:
	 *    - in line  7, P_1 assigns 1 to choosing[1].
	 *    - in line 19, P_1 assigns to number[1] and choosing[1]
	 *    - in line 31, P_1 compares against number[1], and "1 <= j"
	 *    - in line 37, P_1 assigns 0 to number[1]
	 *
	 * @return  DidWhat::ENDED      on error
	 *          DidWhat::NOTHING    when all guards in an 'if' are false
	 *          DidWhat::SOMETHING  otherwise
	 */
	template<typename OUT>
	DidWhat getNextProc(Context* ctx,
						SV& sv,
						const size_t NUM_PROCS,
						Proc& p,
						size_t pid,
						OUT& label)
	{
		//const auto MAGIC_MAX = Proc::MAGIC_MAX;
		constexpr auto LOOK_MA = DidWhat::SOMETHING;
		auto pcCurrent = p.pc;

		label << "[" << pid << "@" << pcCurrent << "] ";

		// p.pc is manually handled in the switch,
		// according to these labels:
		enum {
			NCS = 0,    // "in non-critical section"
			CHOOSE,     // "choose next customer"
			FOR_LOOP,   // "approach critical section"
            CS,         // "in critical section"
			WAIT,       // "critical section busy; wait and goto CHOOSE"
		};

		switch(pcCurrent)
		{
		case NCS : {
			//	choosing[id] = 1; j = 0; max = 0;
			//	goto choose;
			label << "in NCS ";
			sv.choosing[pid] = 1;
			p.j = 0;
			p.max = 0;
			p.pc = CHOOSE;
			return LOOK_MA;
		}
		case CHOOSE : {
			//	if (j<4) {
			//		if (number[j]>max)
			//			max = number[j];
			//		j = j+1;
			//		goto choose;
			//	} else if (j==4 && max<5) {
			//		number[id] = max+1; choosing[id] = 0; j = 0;
			//		goto for_loop;
			//	}
			label << "in CHOOSE ";

//			uint32_t n0;
//            uint32_t c0;
//            number.readMemory(ctx, 0, n0);
//            number.readMemory(ctx, 0, c0);
//            label << "{" << n0 << "," << c0 << "}   ";
			uint32_t v;
			if (p.j < NUM_PROCS) {
				v = sv.number[p.j];
				if(v > p.max)
					p.max = v;
				p.j++;
				p.pc = CHOOSE;
				label << "staying in CHOOSE ";
				return LOOK_MA;
			} else if (p.j == NUM_PROCS && p.max < NUM_PROCS + 1) {
                sv.number[pid] = p.max+1;
                sv.choosing[pid] = 0;
				p.j = 0;
				p.pc = FOR_LOOP;
				label << "going to FOR_LOOP ";
				return LOOK_MA;
			}
            return DidWhat::NOTHING;
		}
		case FOR_LOOP : {
			//	if (j<4 && choosing[j]==0)
			//		goto wait;
			//	else if (j==4)
			//		goto CS; 
			label << "in FOR_LOOP ";
			if (p.j < NUM_PROCS) {
                uint32_t v = sv.choosing[p.j];
                if(v == 0) {
                    label << "going to WAIT ";
                    p.pc = WAIT;
                    return LOOK_MA;
                }
			} else if (p.j == NUM_PROCS) {
				label << "going to CS ";
				p.pc = CS;
				return LOOK_MA;
			}
            return DidWhat::NOTHING;
		}
		case WAIT : {
			//	if (number[j] == 0
			//		|| number[j] < number[id]
			//		|| (number[j] == number[id] && id <= j))
			//	{
			//		j = j+1;
			//		goto for_loop;
			//	}
			label << "in WAIT ";
			uint32_t nj = sv.number[p.j];
			uint32_t nid = sv.number[pid];
			//printf("\nnumber[j]=%u\tnumber[id]=%u\n", nj, nid);  // TODO erase dbg
//			label << "number[j]=" << nj << " ; ";
//			label << "number[id]=" << nid << std::endl;  // TODO erase dbg
			if (nj == 0 || nj < nid || (nj == nid && pid <= p.j)) {
				label << "going to FOR_LOOP ";
				p.j++;
				p.pc = FOR_LOOP;
				return LOOK_MA;
			}
            return DidWhat::NOTHING;
		}
		case CS : {
			//	number[id] = 0;
			//	goto NCS;  -->  we change this, to terminate as "PC_DONE"
			label << "in CS ";
			sv.number[pid] = 0;
			p.pc = NCS;
			return LOOK_MA;
		}
//		case Proc::PC_DONE : {
//			label << "we're done here ";
//			return DidWhat::NOTHING;
//		}
		default:
			abort();
			break;
		}
		return DidWhat::ENDED;
	}

	StateID getInitial(Context* ctx) override {
		Settings& settings = Settings::global();

		uint32_t procs = 4;

		auto initSize = sizeof(SV);
		assert( ! (initSize & 3));

		StateSlot svmem[initSize / 4];
		memset(svmem, 0, initSize);
		SV& sv = *(SV*)svmem;

		//Proc::MAGIC_MAX = procs+1;  // magic!
		for (auto i = procs; i-- ;) {
			Proc& p = sv.proc[i];
			//p.id = i;
			p.max = p.pc = p.j = 0;
		}

		StateIdentifier init;
		init.init(ctx, svmem, initSize / 4, true);
		printf("Uploading initial state %zx\n", init.data);

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

	auto model = new ProcModel();
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
