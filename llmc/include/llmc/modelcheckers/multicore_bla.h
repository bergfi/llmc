#pragma once

#include "interface.h"
#include <atomic>
#include <deque>
#include <thread>
#include <shared_mutex>
#include <iostream>

#include <libfrugi/System.h>
#include <libfrugi/Settings.h>

#include <llmc/common/msq.h>

using namespace libfrugi;

#define tprintf(str, ...) {}
//#define tprintf(str, ...) { \
//    printf(str, __VA_ARGS__); fflush(stdout);\
//}

struct cpuset {

    static cpuset createForNUMACPU(size_t cpu) {
        size_t numCPU = sysconf( _SC_NPROCESSORS_ONLN );
        cpu_set_t* _mask = CPU_ALLOC(numCPU);
        size_t _size = CPU_ALLOC_SIZE(numCPU);
        CPU_ZERO_S(_size, _mask);
        for(size_t t = cpu; t < numCPU; t+=4) {
            CPU_SET_S(t, _size, _mask);
        }
        return cpuset(_size, _mask);
    }

    static cpuset createForCPU(size_t cpu) {
        size_t numCPU = sysconf( _SC_NPROCESSORS_ONLN );
        cpu_set_t* _mask = CPU_ALLOC(numCPU);
        size_t _size = CPU_ALLOC_SIZE(numCPU);
        CPU_ZERO_S(_size, _mask);
        CPU_SET_S(cpu, _size, _mask);
        return cpuset(_size, _mask);
    }

    cpuset(): _size(0), _mask(nullptr) {}
    cpuset(size_t size, cpu_set_t* mask): _size(size), _mask(mask) {}

//    ~cpuset() {
//        if(_mask) {
//            CPU_FREE(_mask);
//        }
//    }

    size_t _size;
    cpu_set_t* _mask;
};

template<typename Model, typename STORAGE, template<typename,typename> typename LISTENER>
class MultiCoreModelChecker: public VModelChecker<llmc::storage::StorageInterface> {
public:
    using mutex_type = std::mutex;
    using read_only_lock  = std::shared_lock<mutex_type>;
    using updatable_lock = std::unique_lock<mutex_type>;
public:

    using Storage = STORAGE;
    using StateSlot = typename Storage::StateSlot;
    using StateTypeID = typename Storage::StateTypeID;
    using StateID = typename Storage::StateID;
    using Delta = typename Storage::Delta;
    using MultiDelta = typename Storage::MultiDelta;
    using FullState = typename Storage::FullState;
    using InsertedState = typename Storage::InsertedState;
    using Listener = LISTENER<MultiCoreModelChecker, Model>;

//    struct Context: public ModelChecker<Model, llmc::storage::StorageInterface>::template ContextImpl<MCI, Model> {
    struct Context: public VContextImpl<llmc::storage::StorageInterface, MultiCoreModelChecker> {
        size_t localStates;
        size_t localTransitions;
        size_t threadID;
        std::deque<StateID> stateQueueNew;
        SimpleAllocator<StateSlot> allocator;

        bool first;
        double elapsedSeconds;
        size_t stateSlotsInsertedIntoStorage;

        Storage::Context storageContext;

        Context(MultiCoreModelChecker* mc, Model* model, size_t threadID): VContextImpl<llmc::storage::StorageInterface, MultiCoreModelChecker>(mc, model), localStates(0), localTransitions(0), threadID(threadID), first(false), stateSlotsInsertedIntoStorage(0) {}
    };

    MultiCoreModelChecker(Model* m): VModelChecker<llmc::storage::StorageInterface>(m), _states(0), _transitions(0), _threads(0), _stats(0) {
        _rootTypeID = 0;
    }

    MultiCoreModelChecker(Model* m, Listener& listener): VModelChecker<llmc::storage::StorageInterface>(m), _states(0), _transitions(0), _centralStateEnqueues(0), _centralStateDequeues(0), _threads(0), _stats(0), _listener(listener) {
        _rootTypeID = 0;
    }

    template<int limit>
    static void* workerThread(void* _ctx) {
        System::Timer timer;
        Context& ctx = *(Context*)_ctx;
        MultiCoreModelChecker& mc = *ctx.getModelChecker();
        Model& model = *ctx.getModel();
        Storage& storage = mc.getStorage();

        ctx.elapsedSeconds = 0.0;

        size_t& tid = ctx.threadID;
        (void)tid;

//        size_t aff = sched_getcpu();

        if constexpr(storage.needsThreadInit()) {
            storage.thread_init();
        }
        tprintf("[%i] starting up thread %u... \n", tid);
        do {
            tprintf("[%i] getting new state from shared queue \n", tid);
            if constexpr(limit == 0) {
                ctx.first = true;
            }
//            size_t aff2 = sched_getcpu();
//            if(aff != aff2) {
//                printf("!!! Affinity changed %zu -> %zu\n", aff, aff2);
//            }
            ctx.elapsedSeconds += timer.getElapsedSeconds();
            StateID current = mc.getNextQueuedState(&ctx);
            timer.reset();
            if(!current.exists()) break;
            tprintf("[%i] got %16zx \n", tid, current);
            do {
                tprintf("[%i] current state: %16zx \n", tid, current);
                ctx.sourceState = current.getData();
                ctx.allocator.clear();
                model.getNextAll(ctx.sourceState, &ctx);
                if(ctx.stateQueueNew.empty()) {
                    break;
                }
                if constexpr(limit != 0) {
                    if(ctx.stateQueueNew.size() >= limit) {
                        return nullptr;
                    }
                }
                current = ctx.stateQueueNew.front().getData();
                ctx.stateQueueNew.pop_front();
            } while(true);
            tprintf("[%i] done all in the local queue, need more states\n", tid);
        } while(true);
        tprintf("[%i] done.\n", tid);
        return nullptr;
    }

    void go() {
        Context ctx(this, this->_m, 0);

        _storage.init();
        if constexpr(Storage::needsThreadInit()) {
            _storage.thread_init();
        }
        _m->init(&ctx);
        StateID init = this->_m->getInitial(&ctx).getData();
        System::Timer timer;

//        stateQueueNew.push_back(init);
        _globalQueue.enqueue(init.getData());
        tprintf("[%i] enqueued initial state...\n", ctx.threadID);
        _centralStateEnqueues.fetch_add(1, std::memory_order_relaxed);
        _states++;
        ctx.stateSlotsInsertedIntoStorage += _storage.determineLength(init);

        printf("first phase...\n");
        _waiters = 0;
        quit = false;

        workerThread<16>((void*)&ctx);

        double threadElapsed = 0;

        bool fireThreads = !ctx.stateQueueNew.empty();

        if(fireThreads) {

            printf("first phase resulted in %zu states and %zu transitions\n", ctx.localStates, ctx.localTransitions);

            _states.fetch_add(ctx.localStates, std::memory_order_relaxed);
            _transitions.fetch_add(ctx.localTransitions, std::memory_order_relaxed);

            ctx.localStates = 0;
            ctx.localTransitions = 0;

            if(_threads == 0) {
                _threads = System::getNumberOfAvailableCores();
            }
            _workers.reserve(_threads);
            _workerContexts.reserve(_threads);
            printf("going (%zu)...\n", _threads);
            quit = false;
            for(auto& s: ctx.stateQueueNew) {
                _globalQueue.enqueue(s.getData());
            }

            // Fire up all threads
            for(size_t tid = 0; tid < _threads; ++tid) {
                cpuset set = cpuset::createForCPU(tid);
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setaffinity_np(&attr, set._size, set._mask);
                _workerContexts.emplace_back(this, this->_m, tid);
                pthread_create(&_workers[tid], &attr, (void* (*)(void*)) workerThread<0>,
                               (void*) &_workerContexts[tid]);
            }

            // Wait for all threads to finish
            for(size_t tid = 0; tid < _threads; ++tid) {
                pthread_join(_workers[tid], nullptr);
            }
        }
        auto elapsed = timer.getElapsedSeconds();

        size_t stateSlotsInsertedIntoStorage = 0;
        if(fireThreads) {
            for(size_t tid=_threads; tid--;) {
                if(!_workerContexts[tid].stateQueueNew.empty()) {
                    printf("State Queue %zu still contains states: %zu\n", tid, _workerContexts[tid].stateQueueNew.size());
                }
                if(_workerContexts[tid].localStates != 0) {
                    printf("State Queue %zu still contains states count: %zu\n", tid, _workerContexts[tid].localStates);
                }
                if(_workerContexts[tid].localTransitions != 0) {
                    printf("State Queue %zu still contains transitions count: %zu\n", tid, _workerContexts[tid].localTransitions);
                }
                threadElapsed += _workerContexts[tid].elapsedSeconds;
                stateSlotsInsertedIntoStorage += _workerContexts[tid].stateSlotsInsertedIntoStorage;
            }
        } else {
            stateSlotsInsertedIntoStorage = ctx.stateSlotsInsertedIntoStorage;
            _threads = 1;
            threadElapsed = elapsed;
        }

        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states.load(std::memory_order_acquire), _transitions.load(std::memory_order_acquire));
        printf("States/s: %lf\n", ((double)_states.load(std::memory_order_acquire))/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions.load(std::memory_order_acquire))/elapsed);
        printf("Thread efficiency: %3.3lf%%\n", 100 * threadElapsed / (_threads * elapsed));
        printf("Central enqueues: %zu (%3.3lf%%)\n", _centralStateEnqueues.load(std::memory_order_relaxed), (double)100 * _centralStateEnqueues.load(std::memory_order_relaxed) / _states);
        printf("Central dequeues: %zu (%3.3lf%%)\n", _centralStateDequeues.load(std::memory_order_relaxed), (double)100 * _centralStateDequeues.load(std::memory_order_relaxed) / _states);
        if(_stats) {
            printf("Storage Stats (%s):\n", _storage.getName().c_str());
            //Stats
            auto stats = _storage.getStatistics();
            _storage.printStats();
            printf("Inserted vector-data in bytes: %zu\n", 4 * stateSlotsInsertedIntoStorage);
            printf("Compression ratio (%zu used bytes", stats._bytesInUse);
            if(stats._bytesInUse < 1024) {
                printf("): ");
            } else if(stats._bytesInUse < 1024*1024) {
                printf(" = ~%.2lfKiB): ", (double) stats._bytesInUse / 1024);
            } else if(stats._bytesInUse < 1024*1024*1024) {
                printf(" = ~%.2lfMiB): ", (double) stats._bytesInUse / 1024 / 1024);
            } else {
                printf(" = ~%.2lfGiB): ", (double) stats._bytesInUse / 1024 / 1024 / 1024);
            }
            printf("%lf (%lf bytes per state)\n", 4 * (double)ctx.stateSlotsInsertedIntoStorage / (stats._bytesInUse), (double)stats._bytesInUse / _states);
            printf("Compression ratio (%zu reserved bytes", stats._bytesReserved);
            if(stats._bytesReserved < 1024) {
                printf("): ");
            } else if(stats._bytesInUse < 1024*1024) {
                printf(" = ~%.2lfKiB): ", (double) stats._bytesReserved / 1024);
            } else if(stats._bytesReserved < 1024*1024*1024) {
                printf(" = ~%.2lfMiB): ", (double) stats._bytesReserved / 1024 / 1024);
            } else {
                printf(" = ~%.2lfGiB): ", (double) stats._bytesReserved / 1024 / 1024 / 1024);
            }
            printf("%lf (%lf bytes per state)\n", 4 * (double)ctx.stateSlotsInsertedIntoStorage / (stats._bytesReserved), (double)stats._bytesReserved / _states);
        }
    }

    StateID getNextQueuedState(Context* ctx) {
        _states.fetch_add(ctx->localStates, std::memory_order_relaxed);
        _transitions.fetch_add(ctx->localTransitions, std::memory_order_relaxed);

        ctx->localStates = 0;
        ctx->localTransitions = 0;

        size_t s;

        // This is a while because at the start there is a race to get an item out of the queue
        while(!_globalQueue.dequeue(s)) {
            updatable_lock lock(mtx);
            size_t localWaiters = _waiters;
            tprintf("[%i] Blockers: %zu (threads: %zu)\n", ctx->threadID, localWaiters, _threads);
            if(localWaiters == _threads - 1) {
                tprintf("[%i] signalling quit...\n", ctx->threadID);
                quit = true;
                std::atomic_thread_fence(std::memory_order_seq_cst);
                cv.notify_all();
                return StateID::NotFound();
            }
            ++_waiters;
            cv.wait(lock);
            --_waiters;
            if(quit) {
                return StateID::NotFound();
            }
        }

        tprintf("[%i] dequeued... %zu\n", ctx->threadID, s);
        return s;
    }

    void queueState(Context* ctx, StateID stateID) {
//        _centralStateEnqueues.fetch_add(1, std::memory_order_relaxed);
        tprintf("[%i] added to shared queue: %16xu\n", ctx->threadID, stateID);
        _globalQueue.enqueue(stateID.getData());
        cv.notify_one();
    }

    llmc::storage::StorageInterface::InsertedState newState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateTypeID const& typeID, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        auto insertedState = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well
        //TODO ctx->stateSlotsInsertedIntoStorage += _storage.determineLength(insertedState.getState());

        return llmc::storage::StorageInterface::InsertedState(insertedState.getState().getData(), insertedState.isInserted());
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);

        // the type ID could be extracted from stateID...
        auto insertedState = newState(ctx_, _rootTypeID, length, slots);
        if (insertedState.isInserted()) {
            ctx->localStates++;
            if (ctx->first) {
                queueState(ctx, insertedState.getState().getData());
                ctx->first = false;
            } else {
                ctx->stateQueueNew.push_back(insertedState.getState());
                tprintf("[%i] added to local queue: %16xu\n", ctx->threadID, insertedState.getState());
            }
            ctx->stateSlotsInsertedIntoStorage += _storage.determineLength(insertedState.getState());
        } else {
            tprintf("[%i] already visited: %16xu\n", ctx->threadID, insertedState.getState());
        }
        ctx->localTransitions++;
        return insertedState.getState().getData();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::MultiDelta const& delta, TransitionInfoUnExpanded const& tinfo) override {
//        Context *ctx = static_cast<Context *>(ctx_);
        abort();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::Delta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        Context* ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = *reinterpret_cast<StateID const*>(&ctx->sourceState);
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
            ctx->localStates++;
            if(ctx->first) {
                queueState(ctx, insertedState.getState());
                ctx->first = false;
            } else{
                ctx->stateQueueNew.push_back(insertedState.getState().getData());
                tprintf("[%i] added to local queue: %16xu\n", ctx->threadID, insertedState.getState());
            }
            ctx->stateSlotsInsertedIntoStorage += _storage.determineLength(insertedState.getState());
        }
        ctx->localTransitions++;
        return insertedState.getState().getData();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t offset, size_t length, const StateSlot* slots, TransitionInfoUnExpanded const& transition) override {
        Context* ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = *reinterpret_cast<StateID const*>(&ctx->sourceState);
        auto insertedState = _storage.insert(stateID, offset, length, slots, true);
        if(insertedState.isInserted()) {
            ctx->localStates++;
            if(ctx->first) {
                queueState(ctx, insertedState.getState());
                ctx->first = false;
            } else{
                ctx->stateQueueNew.push_back(insertedState.getState().getData());
                tprintf("[%i] added to local queue: %16xu\n", ctx->threadID, insertedState.getState());
            }
            ctx->stateSlotsInsertedIntoStorage += _storage.determineLength(insertedState.getState());
        }
        ctx->localTransitions++;
        return insertedState.getState().getData();
    }

    llmc::storage::StorageInterface::FullState* getState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context* ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, true);
        } else {
            size_t stateLength = _storage.determineLength(s);
            llmc::storage::StorageInterface::FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(llmc::storage::FullStateData<StateSlot>) / sizeof(StateSlot) + stateLength);
            llmc::storage::StorageInterface::FullState::create(dest, true, stateLength);
            _storage.get(dest->getDataToModify(), s, true);
            return dest;
        }
    }

    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        auto insertedState = _storage.insert(slots, length, false);
        assert(insertedState.getState().getData());
        return insertedState.getState();
    }
    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID_, llmc::storage::StorageInterface::Delta const& delta) override {
        StateID const& stateID = *reinterpret_cast<StateID const*>(&stateID_);
        auto insertedState = _storage.insert(stateID, delta, false);
        return insertedState.getState().getData();
    }
    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, size_t offset, size_t length, const llmc::storage::StorageInterface::StateSlot* data) override {
        auto insertedState = _storage.insert(stateID, offset, length, data, false);
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::FullState* getSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context* ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, false);
        } else {
            size_t stateLength = _storage.determineLength(s);
            llmc::storage::StorageInterface::FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(llmc::storage::FullStateData<StateSlot>) / sizeof(StateSlot) + stateLength);
            llmc::storage::StorageInterface::FullState::create(dest, false, stateLength);
            _storage.get(dest->getDataToModify(), s, false);
            return dest;
        }
    }

    virtual bool getStatePartial(VContext<llmc::storage::StorageInterface>* ctx_, StateID const& s, size_t offset, StateSlot* data, size_t length, bool isRoot = true) {
        (void)ctx_;
        return _storage.getPartial(s, offset, data, length, isRoot);
    }

    virtual bool getSubStatePartial(VContext<llmc::storage::StorageInterface>* ctx_, StateID const& s, size_t offset, StateSlot* data, size_t length) {
        (void)ctx_;
        return _storage.getPartial(s, offset, data, length, false);
    }

    //    bool getState(StateSlot* dest, StateID const& s) {
//        return _storage.get(dest, s, true);
//    }

    bool newType(llmc::storage::StorageInterface::StateTypeID typeID, std::string const& name) override {
        abort();
        return false;
    }

//    llmc::storage::StorageInterface::Delta* newDelta(size_t offset, llmc::storage::StorageInterface::StateSlot* data, size_t len) override {
//        return Delta::create(offset, data, len);
//    }

    llmc::storage::StorageInterface::Delta& newDelta(void* buffer, size_t offset, const StateSlot* data, size_t len) override {
        return llmc::storage::StorageInterface::Delta::create(buffer, offset, data, len);
    }

    void deleteDelta(llmc::storage::StorageInterface::Delta* d) override {
        return Delta::destroy(d);
    }

    llmc::storage::StorageInterface::StateTypeID newType(std::string const& name) override {
        static size_t t = 1;
        return t++;
    }
    bool setRootType(llmc::storage::StorageInterface::StateTypeID typeID) override {
        _rootTypeID = typeID;
        return true;
    }

    STORAGE& getStorage() { return _storage; }

    size_t getStates() const {
        return _states;
    }

    size_t getTransitions() const {
        return _transitions;
    }

    void setSettings(Settings& settings) {
        _threads = settings["threads"].asUnsignedValue();
        if(settings["nostats"].asUnsignedValue()) _stats = 0;
    }

protected:
    std::mutex mtx;
    std::condition_variable cv;
    bool quit;
    std::atomic<size_t> _states;
    std::atomic<size_t> _transitions;
    std::atomic<size_t> _centralStateEnqueues;
    std::atomic<size_t> _centralStateDequeues;
    StateTypeID _rootTypeID;
    Storage _storage;
    Listener& _listener;
    size_t _threads;
    size_t _stats;
    std::vector<Context> _workerContexts;
    std::vector<pthread_t> _workers;
    common::MSQ _globalQueue;
    size_t _waiters;
};