#pragma once

#include "interface.h"
#include <atomic>
#include <deque>
#include <thread>
#include <shared_mutex>
#include <iostream>

#include <libfrugi/System.h>

#define tprintf(str, ...) {}

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
    struct Context: public VContext<llmc::storage::StorageInterface> {
        std::deque<StateID> stateQueueNew;
        size_t threadID;
        SimpleAllocator<StateSlot> allocator;

        bool first;

        Context(MultiCoreModelChecker* mc, Model* model): VContext<llmc::storage::StorageInterface>(mc, model) {}
    };

    MultiCoreModelChecker(Model* m): VModelChecker<llmc::storage::StorageInterface>(m), _states(0) {
        _rootTypeID = 0;
    }

    MultiCoreModelChecker(Model* m, Listener& listener): VModelChecker<llmc::storage::StorageInterface>(m), _states(0), _listener(listener) {
        _rootTypeID = 0;
    }

    void go() {
        Context ctx(this, this->_m);
        _storage.init();
        _m->init(&ctx);
        StateID init = this->_m->getInitial(&ctx).getData();
        System::Timer timer;

        stateQueueNew.push_back(init);

        waiters = 0;
        quit = false;

        printf("going...\n");

        // vector container stores threads
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; i++) {
            workers.push_back(std::thread([this,i]()
            {
                if constexpr(_storage.needsThreadInit()) {
                    _storage.thread_init();
                }
                tprintf("[%i] starting up... \n", i);
                Context ctx(this, this->_m);
                ctx.threadID = i;
                do {
                  tprintf("[%i] getting new state from shared queue \n", i);
                  StateID current = getNextQueuedState(&ctx);
                  if(!current.exists()) break;
                  tprintf("[%i] got %16zx \n", i, current);
                  do {
                      tprintf("[%i] current state: %16zx \n", i, current);
                      ctx.sourceState = current.getData();
                      ctx.allocator.clear();
                      ctx.first = true;
                      this->_m->getNextAll(ctx.sourceState, &ctx);
                      size_t statesLocal = _states.load(std::memory_order_relaxed);
                      size_t statesNew = ctx.stateQueueNew.size();
                      while(_states.compare_exchange_weak(statesLocal, statesLocal + statesNew + 1));
                      if(ctx.stateQueueNew.empty()) break;
                      current = ctx.stateQueueNew.front().getData();
                      ctx.stateQueueNew.pop_front();
                  } while(true);
                  tprintf("[%i] done all in the local queue\n", i);
                  size_t statesLocal = _states.load(std::memory_order_relaxed);
//                  if(statesLocal >= 100000) {
//                      tprintf("[%i] aborting after %zu states\n", i, statesLocal);
//                      fflush(stdout);
//                      break;
//                  }
                  tprintf("[%i] need more states... \n", i);
                } while(true);
                tprintf("[%i] done.\n", i);
            }
            ));
        }
        for (int i = 0; i < 4; i++) {
            workers[i].join();
        }
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states.load(std::memory_order_acquire), _transitions.load(std::memory_order_acquire));
        printf("States/s: %lf\n", ((double)_states.load(std::memory_order_acquire))/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions.load(std::memory_order_acquire))/elapsed);
        printf("Storage Stats:\n");
        _storage.printStats();
    }

    StateID getNextQueuedState(Context* ctx) {
        updatable_lock lock(mtx);
        if(stateQueueNew.empty()) {
            if(waiters == 3) {
                tprintf("[%i] signalling quit...\n", ctx->threadID);
                quit = true;
                queue_cv.notify_all();
            } else {
                tprintf("[%i] waiting...\n", ctx->threadID);
                queue_cv.wait(lock);
                tprintf("[%i] waking up...\n", ctx->threadID);
            }
            if(quit) {
                tprintf("[%i] quitting...\n", ctx->threadID);
                return StateID::NotFound();
            }
            if(stateQueueNew.empty()) {
                tprintf("[%i] wtf...\n", ctx->threadID);
            }
        }
        auto result = stateQueueNew.front();
        stateQueueNew.pop_front();
        return result;
    }

    void queueState(Context* ctx, StateID stateID) {
        updatable_lock lock(mtx);
        tprintf("[%i] added to shared queue: %16xu", ctx->threadID, stateID);
        stateQueueNew.push_back(stateID);
        if(stateQueueNew.size() == 1) {
            tprintf("[%i] notifying...\n", ctx->threadID);
            queue_cv.notify_one();
        }
    }

    llmc::storage::StorageInterface::InsertedState newState(llmc::storage::StorageInterface::StateTypeID const& typeID, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        auto insertedState = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well

        return llmc::storage::StorageInterface::InsertedState(insertedState.getState().getData(), insertedState.isInserted());
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);

        // the type ID could be extracted from stateID...
        auto insertedState = newState(_rootTypeID, length, slots);
        if (insertedState.isInserted()) {
            if (ctx->first) {
                queueState(ctx, insertedState.getState().getData());
                ctx->first = false;
            } else {
                ctx->stateQueueNew.push_back(insertedState.getState());
            }
        }
        _transitions++;
        return insertedState.getState().getData();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::MultiDelta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        _transitions++;
        abort();
        return 0;
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::Delta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        Context* ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = *reinterpret_cast<StateID const*>(&ctx->sourceState);
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
            if(ctx->first) {
                queueState(ctx, insertedState.getState());
                ctx->first = false;
            } else{
                ctx->stateQueueNew.push_back(insertedState.getState().getData());
            }
        }
        _transitions++;
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

    llmc::storage::StorageInterface::StateID newSubState(llmc::storage::StorageInterface::StateID const& stateID_, llmc::storage::StorageInterface::Delta const& delta) override {
        StateID const& stateID = *reinterpret_cast<StateID const*>(&stateID_);
        auto insertedState = _storage.insert(stateID, delta, false);
        return insertedState.getState().getData();
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

//    bool getState(StateSlot* dest, StateID const& s) {
//        return _storage.get(dest, s, true);
//    }

    bool newType(llmc::storage::StorageInterface::StateTypeID typeID, std::string const& name) override {
        abort();
        return false;
    }

    llmc::storage::StorageInterface::Delta* newDelta(size_t offset, llmc::storage::StorageInterface::StateSlot* data, size_t len) override {
        return Delta::create(offset, data, len);
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

protected:
    mutex_type mtx;
    std::condition_variable queue_cv;
    size_t waiters;
    bool quit;
    std::deque<StateID> stateQueueNew;
    std::atomic<size_t> _states;
    std::atomic<size_t> _transitions;
    StateTypeID _rootTypeID;
    Storage _storage;
    Listener& _listener;
};