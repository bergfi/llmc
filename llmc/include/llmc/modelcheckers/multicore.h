#pragma once

#include "interface.h"
#include <atomic>
#include <deque>
#include <thread>
#include <shared_mutex>
#include <iostream>

#include <libfrugi/System.h>

#define tprintf(str, ...) {}

template<template<typename> typename MODEL, typename STORAGE>
class MultiCoreModelChecker: public ModelChecker<MODEL<MultiCoreModelChecker<MODEL, STORAGE>>, STORAGE> {
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
    using Model = MODEL<MultiCoreModelChecker>;

    struct Context: public ModelChecker<Model, STORAGE>::Context {
        std::deque<StateID> stateQueueNew;
        size_t threadID;

        bool first;

        Context(MultiCoreModelChecker* mc_, void* model): ModelChecker<Model, STORAGE>::Context(mc_, model) {}
    };

    MultiCoreModelChecker(Model* m):  ModelChecker<Model, STORAGE>(m), _states(0) {
        _rootTypeID = 0;
    }

    void go() {
        _storage.init();
        Context ctx(this, (void*)this->_m);
        StateID init = this->_m->getInitial(ctx);
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
                                              tprintf("[%i] starting up... \n", i);
                                              Context ctx(this, (void*)this->_m);
                                              ctx.threadID = i;
                                              do {
                                                  tprintf("[%i] getting new state from shared queue \n", i);
                                                  StateID current = getNextQueuedState(&ctx);
                                                  if(!current.exists()) break;
                                                  tprintf("[%i] got %16zx \n", i, current);
                                                  do {
                                                      tprintf("[%i] current state: %16zx \n", i, current);
                                                      ctx.sourceState = current;
                                                      ctx.allocator.clear();
                                                      ctx.first = true;
                                                      this->_m->getNextAll(current, ctx);
                                                      size_t statesLocal = _states.load(std::memory_order_relaxed);
                                                      size_t statesNew = ctx.stateQueueNew.size();
                                                      while(_states.compare_exchange_weak(statesLocal, statesLocal + statesNew + 1));
                                                      if(ctx.stateQueueNew.empty()) break;
                                                      current = ctx.stateQueueNew.front();
                                                      ctx.stateQueueNew.pop_front();
                                                  } while(true);
                                                  tprintf("[%i] done all in the local queue\n", i);
                                                  size_t statesLocal = _states.load(std::memory_order_relaxed);
                                                  if(statesLocal >= 100000) {
                                                      tprintf("[%i] aborting after %zu states\n", i, statesLocal);
                                                      fflush(stdout);
                                                      break;
                                                  }
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

    virtual InsertedState newState(StateTypeID const& typeID, size_t length, StateSlot* slots) {
        auto r = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well

        return r;
    }

    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, size_t length, StateSlot* slots) {
        Context *ctx = static_cast<Context *>(ctx_);

        // the type ID could be extracted from stateID...
        auto insertedState = newState(_rootTypeID, length, slots);
        if (insertedState.isInserted()) {
            if (ctx->first) {
                queueState(ctx, insertedState.getState());
                ctx->first = false;
            } else {
                ctx->stateQueueNew.push_back(insertedState.getState());
            }
        }
        _transitions++;
        return insertedState.getState();
    }

    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, MultiDelta const& delta) {
        _transitions++;
        abort();
        return 0;
    }

    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, Delta const& delta) {
        Context* ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
            if(ctx->first) {
                queueState(ctx, insertedState.getState());
                ctx->first = false;
            } else{
                ctx->stateQueueNew.push_back(insertedState.getState());
            }
        }
        _transitions++;
        return insertedState.getState();
    }

    virtual FullState* getState(typename ModelChecker<Model, STORAGE>::Context* ctx_, StateID const& s) {
        Context* ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, true);
        } else {
            FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(FullStateData<StateSlot>) / sizeof(StateSlot) + s.getLength());
            FullState::create(dest, true, s.getLength());
            _storage.get(dest->getData(), s, true);
            return dest;
        }
    }

    virtual StateID newSubState(StateID const& stateID, Delta const& delta) {
        auto insertedState = _storage.insert(stateID, delta, false);
        return insertedState.getState();
    }

    virtual FullState* getSubState(typename ModelChecker<Model, STORAGE>::Context* ctx_, StateID const& s) {
        Context* ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, false);
        } else {
            FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(FullStateData<StateSlot>) / sizeof(StateSlot) + s.getLength());
            FullState::create(dest, false, s.getLength());
            _storage.get(dest->getData(), s, false);
            return dest;
        }
    }

//    virtual bool getState(StateSlot* dest, StateID const& s) {
//        return _storage.get(dest, s, true);
//    }

    virtual bool newType(StateTypeID typeID, std::string const& name) {
        abort();
        return false;
    }

    virtual Delta* newDelta(size_t offset, StateSlot* data, size_t len) {
        return Delta::create(offset, data, len);
    }

    virtual void deleteDelta(Delta* d) {
        return Delta::destroy(d);
    }

    virtual StateTypeID newType(std::string const& name) {
        static size_t t = 1;
        return t++;
    }
    virtual bool setRootType(StateTypeID typeID) {
        _rootTypeID = typeID;
        return true;
    }

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
};