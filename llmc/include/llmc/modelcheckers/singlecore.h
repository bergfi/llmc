#pragma once

template<template<typename> typename MODEL, typename STORAGE>
class SingleCoreModelChecker: public ModelChecker<MODEL<SingleCoreModelChecker<MODEL, STORAGE>>, STORAGE> {
public:

    using Storage = STORAGE;
    using StateSlot = typename Storage::StateSlot;
    using StateTypeID = typename Storage::StateTypeID;
    using StateID = typename Storage::StateID;
    using Delta = typename Storage::Delta;
    using MultiDelta = typename Storage::MultiDelta;
    using FullState = typename Storage::FullState;
    using InsertedState = typename Storage::InsertedState;
    using Model = MODEL<SingleCoreModelChecker>;
    using Context = typename ModelChecker<Model, STORAGE>::Context;

    SingleCoreModelChecker(Model* m):  ModelChecker<Model, STORAGE>(m) {
        _rootTypeID = 0;
    }

    void go() {
        _storage.init();
        Context ctx(this, (void*)this->_m);
        std::cout << this << std::endl;
        std::cout << ctx.mc << std::endl;
        StateID init = this->_m->getInitial(ctx);
        System::Timer timer;
        if(init.exists()) {
            stateQueueNew.emplace_back(init);
            int level = 0;

            _states++;

            do {
                int level_states = 0;
                stateQueueNew.swap(stateQueue);
                do {
                    StateID s = stateQueue.front();
                    stateQueue.pop_front();
                    ctx.sourceState = s;
                    ctx.allocator.clear();
                    this->_m->getNextAll(s, ctx);
                    level_states++;
                } while(!stateQueue.empty());
                //if (!(level % 10)) printf("Level %u has %u states\n", level, level_states); fflush(stdout);
                if (_states >= 100000) {
                    printf("aborting after %zu states\n", _states);
                    fflush(stdout);
                    goto done;
                }
                level++;
            } while(!stateQueueNew.empty());
        }
        done:
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states, _transitions);
        printf("States/s: %lf\n", ((double)_states)/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Storage Stats:\n");
        _storage.printStats();
    }

    virtual InsertedState newState(StateTypeID const& typeID, size_t length, StateSlot* slots) {
        auto r = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?
        printf("new state %16zx %u\n", r.getState(), r.isInserted());
        return r;
    }
    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, size_t length, StateSlot* slots) {
        StateID const& stateID = ctx_->sourceState;
        // the type ID could be extracted from stateID...
        auto insertedState = newState(_rootTypeID, length, slots);
        if(insertedState.isInserted()) {
            printf("new transition %16zx -> %16zx\n", stateID, insertedState.getState());
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }
    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, MultiDelta const& delta) {
        _transitions++;
        return 0;
    }
    virtual StateID newTransition(typename ModelChecker<Model, STORAGE>::Context* ctx_, Delta const& delta) {
        StateID const& stateID = ctx_->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
            printf("new transition %16zx -> %16zx\n", stateID, insertedState.getState());
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    virtual FullState* getState(Context* ctx, StateID const& s) {
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
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    virtual const FullState* getSubState(Context* ctx, StateID const& s) {
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
    StateTypeID _rootTypeID;
    size_t _states = 0;
    size_t _transitions = 0;
    std::deque<StateID> stateQueue;
    std::deque<StateID> stateQueueNew;
    Storage _storage;
};