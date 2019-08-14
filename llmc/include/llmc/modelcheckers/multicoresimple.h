#pragma once

template<template<typename> typename MODEL, typename STORAGE>
class MultiCoreModelCheckerSimple: public ModelChecker<MODEL<MultiCoreModelCheckerSimple<MODEL, STORAGE>>, STORAGE> {
public:
    using mutex_type = std::shared_timed_mutex;
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
    using Model = MODEL<MultiCoreModelCheckerSimple>;
    using Context = typename ModelChecker<Model, STORAGE>::template ContextImpl<MultiCoreModelCheckerSimple, MODEL<MultiCoreModelCheckerSimple<MODEL, STORAGE>>>;

    MultiCoreModelCheckerSimple(Model* m):  ModelChecker<Model, STORAGE>(m), _states(0) {
        _rootTypeID = 0;
    }

    void go() {
        _storage.init();
        Context ctx(this, this->_m);
        StateID init = this->_m->getInitial(ctx);
        System::Timer timer;

        stateQueueNew.push_back(init);

        // vector container stores threads
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; i++) {
            workers.push_back(std::thread([this]()
                                          {
                                              Context ctx(this, this->_m);
                                              do {
                                                  StateID current = getNextQueuedState();
                                                  if(!current.exists()) break;
                                                  do {
                                                      ctx.sourceState = current;
                                                      ctx.allocator.clear();
                                                      this->_m->getNextAll(current, ctx);
                                                      current = StateID::NotFound();
                                                  } while(current.exists());
                                                  size_t statesLocal = _states;
                                                  if(statesLocal >= 100000) {
                                                      printf("aborting after %zu states\n", statesLocal);
                                                      fflush(stdout);
                                                      break;
                                                  }
                                              } while(true);
                                          }
            ));
        }
        for (int i = 0; i < 4; i++) {
            workers[i].join();
        }
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states, _transitions);
        printf("States/s: %lf\n", ((double)_states)/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Storage Stats:\n");
        _storage.printStats();
    }

    StateID getNextQueuedState() {
        updatable_lock lock(mtx);
        if(stateQueueNew.empty()) return StateID::NotFound();
        auto result = stateQueueNew.front();
        stateQueueNew.pop_front();
        return result;
    }

    InsertedState newState(StateTypeID const& typeID, size_t length, StateSlot* slots) {
        auto r = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well

        return r;
    }
    StateID newTransition(Context* ctx_, size_t length, StateSlot* slots) {
        StateID const& stateID = ctx_->sourceState;

        // the type ID could be extracted from stateID...
        auto insertedState = newState(_rootTypeID, length, slots);
        updatable_lock lock(mtx);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }
    StateID newTransition(Context* ctx_, MultiDelta const& delta) {
        StateID const& stateID = ctx_->sourceState;
        _transitions++;
        abort();
        return 0;
    }
    StateID newTransition(Context* ctx_, Delta const& delta) {
        StateID const& stateID = ctx_->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        updatable_lock lock(mtx);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    FullState* getState(Context* ctx, StateID const& s) {
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, true);
        } else {
            FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(FullStateData<StateSlot>) / sizeof(StateSlot) + s.getLength());
            FullState::create(dest, true, s.getLength());
            _storage.get(dest->getData(), s, true);
            return dest;
        }
    }

    StateID newSubState(StateID const& stateID, Delta const& delta) {
        auto insertedState = _storage.insert(stateID, delta, false);
        return insertedState.getState();
    }

    const FullState* getSubState(Context* ctx, StateID const& s) {
        if constexpr(Storage::accessToStates()) {
            return _storage.get(s, false);
        } else {
            FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(FullStateData<StateSlot>) / sizeof(StateSlot) + s.getLength());
            FullState::create(dest, false, s.getLength());
            _storage.get(dest->getData(), s, false);
            return dest;
        }
    }

//    bool getState(StateSlot* dest, StateID const& s) {
//        return _storage.get(dest, s, true);
//    }

    bool newType(StateTypeID typeID, std::string const& name) {
        return false;
    }

    Delta* newDelta(size_t offset, StateSlot* data, size_t len) {
        return Delta::create(offset, data, len);
    }

    void deleteDelta(Delta* d) {
        return Delta::destroy(d);
    }

    StateTypeID newType(std::string const& name) {
        static size_t t = 1;
        return t++;
    }
    bool setRootType(StateTypeID typeID) {
        _rootTypeID = typeID;
        return true;
    }

protected:
    mutex_type mtx;
    std::deque<StateID> stateQueueNew;
    size_t _states;
    size_t _transitions;
    StateTypeID _rootTypeID;
    Storage _storage;
};