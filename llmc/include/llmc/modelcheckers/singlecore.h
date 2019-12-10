#pragma once

#include <llmc/statespace/listener.h>

template<typename Model, typename STORAGE, template<typename,typename> typename LISTENER>
class SingleCoreModelChecker: public VModelChecker<llmc::storage::StorageInterface> {
public:

    using Storage = STORAGE;
    using StateSlot = typename Storage::StateSlot;
    using StateTypeID = typename Storage::StateTypeID;
    using StateID = typename Storage::StateID;
    using Delta = typename Storage::Delta;
    using MultiDelta = typename Storage::MultiDelta;
    using FullState = typename Storage::FullState;
    using InsertedState = typename Storage::InsertedState;
    using Listener = LISTENER<SingleCoreModelChecker, Model>;
//    using Context = typename ModelChecker<Model, llmc::storage::StorageInterface>::template ContextImpl<MCI, Model>;
    struct Context: public VContext<llmc::storage::StorageInterface> {
        SimpleAllocator<StateSlot> allocator;
        Context(VModelChecker<llmc::storage::StorageInterface>* mc, VModel<llmc::storage::StorageInterface>* model): VContext<llmc::storage::StorageInterface>(mc, model) {}
    };

    explicit SingleCoreModelChecker(Model* m): VModelChecker<llmc::storage::StorageInterface>(m) {
        _rootTypeID = 0;
    }

    SingleCoreModelChecker(Model* m, Listener& listener): VModelChecker<llmc::storage::StorageInterface>(m), _listener(listener) {
        _rootTypeID = 0;
    }

    void go() {
        _listener.init();
        _storage.init();
        Context ctx(this, this->_m);
        StateID init = this->_m->getInitial(&ctx);
        System::Timer timer;
        if(init.exists()) {
            stateQueueNew.emplace_back(init);
            int level = 0;

            _states++;
            size_t statesLastCheck = _states;

            do {
                printf("level %i has %i states\n", level, _states);
                level++;
                int level_states = 0;
                stateQueueNew.swap(stateQueue);
                do {
                    StateID s = stateQueue.front();
                    stateQueue.pop_front();
                    ctx.sourceState = s;
                    ctx.allocator.clear();
//                    printf("=== %zu\n", s);
                    this->_m->getNextAll(ctx.sourceState, &ctx);
                    level_states++;
                } while(!stateQueue.empty());
                //if (!(level % 10)) printf("Level %u has %u states\n", level, level_states); fflush(stdout);
//                if (_states >= 100000) {
//                    printf("aborting after %zu states\n", _states);
//                    fflush(stdout);
//                    goto done;
//                }
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
        _listener.finish();
    }

    llmc::storage::StorageInterface::InsertedState newState(llmc::storage::StorageInterface::StateTypeID const& typeID, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        auto insertedState = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?
//        printf("new state %16zx %u\n", insertedState.getState(), insertedState.isInserted());
        if(insertedState.isInserted()) {
            _listener.writeState(this->getModel(), insertedState.getState(), length, slots);
        }
        return llmc::storage::StorageInterface::InsertedState(insertedState.getState(), insertedState.isInserted());
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        StateID const& stateID = ctx->sourceState;
        // the type ID could be extracted from stateID...
        auto insertedState = newState(_rootTypeID, length, slots);
        if(insertedState.isInserted()) {
//            printf("new transition %16zu -> %16u\n", stateID, insertedState.getState());
            _states++;
            stateQueueNew.push_back(insertedState.getState());
            _listener.writeState(this->getModel(), insertedState.getState(), length, slots);
            _listener.writeTransition(stateID, insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::MultiDelta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        _transitions++;
        abort();
        return 0;
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::Delta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
//            printf("new transition %16zx -> %16zx\n", stateID, insertedState.getState());
            _states++;
            stateQueueNew.push_back(insertedState.getState());
            auto fsd = getState(ctx_, insertedState.getState());
            _listener.writeState(this->getModel(), insertedState.getState(), fsd->getLength(), fsd->getData());
            _listener.writeTransition(stateID, insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::FullState* getState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context *ctx = static_cast<Context *>(ctx_);
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

    llmc::storage::StorageInterface::StateID newSubState(llmc::storage::StorageInterface::StateID const& stateID, Delta const& delta) override {
        auto insertedState = _storage.insert(stateID, delta, false);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    const FullState* getSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context *ctx = static_cast<Context *>(ctx_);
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

protected:
    StateTypeID _rootTypeID;
    size_t _states = 0;
    size_t _transitions = 0;
    std::deque<StateID> stateQueue;
    std::deque<StateID> stateQueueNew;
    Storage _storage;
    Listener& _listener;
};