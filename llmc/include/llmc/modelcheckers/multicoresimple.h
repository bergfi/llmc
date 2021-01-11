#pragma once

#include <deque>
#include <shared_mutex>
#include <thread>
#include <llmc/statespace/listener.h>
#include <libfrugi/System.h>
#include <libfrugi/Settings.h>

using namespace libfrugi;

template<typename Model, typename STORAGE, template<typename,typename> typename LISTENER=llmc::statespace::VoidPrinter>
class MultiCoreModelCheckerSimple: public VModelChecker<llmc::storage::StorageInterface> {
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
    using Listener = LISTENER<MultiCoreModelCheckerSimple, Model>;
    //using Context = typename ModelChecker<Model, StorageInterface>::template ContextImpl<MCI, Model>;
    struct Context: public VContext<llmc::storage::StorageInterface> {
        SimpleAllocator<StateSlot> allocator;
        Context(VModelChecker<llmc::storage::StorageInterface>* mc, VModel<llmc::storage::StorageInterface>* model): VContext<llmc::storage::StorageInterface>(mc, model) {}
    };

    MultiCoreModelCheckerSimple(Model* m):  VModelChecker<llmc::storage::StorageInterface>(m), _states(0), _transitions(0) {
        _rootTypeID = 0;
    }

    MultiCoreModelCheckerSimple(Model* m, Listener& listener):  VModelChecker<llmc::storage::StorageInterface>(m), _states(0), _transitions(0), _listener(listener) {
        _rootTypeID = 0;
    }

    void go() {
//        if constexpr(_storage.stateHasFixedLength()) {
//            _storage.setStateLength(_m->getStateLength());
//        }
        _storage.init();
        Context ctx(this, this->_m);
        _m->init(&ctx);
        StateID init = this->_m->getInitial(&ctx).getData();
        System::Timer timer;

        _states++;

        stateQueueNew.push_back(init);

        // vector container stores threads
        std::vector<std::thread> workers;
        for (size_t i = 0; i < _threads; i++) {
            workers.push_back(std::thread([this]()
                                          {
                                              Context ctx(this, this->_m);
                                              do {
                                                  StateID current = getNextQueuedState();
                                                  if(!current.exists()) break;
                                                  do {
                                                      ctx.sourceState = current.getData();
                                                      ctx.allocator.clear();
                                                      this->_m->getNextAll(ctx.sourceState, &ctx);
                                                      current = StateID::NotFound();
                                                  } while(current.exists());
//                                                  size_t statesLocal = _states;
//                                                  if(statesLocal >= 100000) {
//                                                      printf("aborting after %zu states\n", statesLocal);
//                                                      fflush(stdout);
//                                                      break;
//                                                  }
                                              } while(true);
                                          }
            ));
        }
        for(auto& worker: workers) {
            worker.join();
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

    llmc::storage::StorageInterface::InsertedState newState(VContext<llmc::storage::StorageInterface>* ctx_, StateTypeID const& typeID, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        auto insertedState = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well
        if(insertedState.isInserted()) {
//            _listener.writeState(this->getModel(), insertedState.getState(), length, slots);
        }

        return insertedState;
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
//        auto ctx = static_cast<Context*>(ctx_);
//        StateID const& stateID = ctx->sourceState;

        // the type ID could be extracted from stateID...
        auto insertedState = newState(ctx_, _rootTypeID, length, slots);
        updatable_lock lock(mtx);
        if(insertedState.isInserted()) {
            _states++;
//            _listener.writeState(this->getModel(), insertedState.getState(), length, slots);
            stateQueueNew.push_back(insertedState.getState());
        }
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::MultiDelta const& delta, TransitionInfoUnExpanded const& tinfo) override {
//        auto ctx = static_cast<Context*>(ctx_);
//        StateID const& stateID = ctx->sourceState;
//        _transitions++;
        abort();
        return 0;
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::Delta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        auto ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        updatable_lock lock(mtx);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
//            auto fsd = getState(ctx_, insertedState.getState());
//            _listener.writeState(this->getModel(), insertedState.getState(), fsd->getLength(), fsd->getData());
        }
//        _listener.writeTransition(stateID, insertedState.getState(), _m->getTransitionInfo(ctx, tinfo));
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t offset, size_t length, const StateSlot* slots, TransitionInfoUnExpanded const& transition) override {
        auto ctx = static_cast<Context*>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, offset, length, slots, true);
        updatable_lock lock(mtx);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
//            auto fsd = getState(ctx_, insertedState.getState());
//            _listener.writeState(this->getModel(), insertedState.getState(), fsd->getLength(), fsd->getData());
        }
//        _listener.writeTransition(stateID, insertedState.getState(), _m->getTransitionInfo(ctx, tinfo));
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::FullState* getState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        auto ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            (void)ctx;
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
    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, Delta const& delta) override {
        auto insertedState = _storage.insert(stateID, delta, false);
        return insertedState.getState();
    }
    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, size_t offset, size_t length, const llmc::storage::StorageInterface::StateSlot* data) override {
        auto insertedState = _storage.insert(stateID, offset, length, data, false);
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID appendState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, size_t length, const llmc::storage::StorageInterface::StateSlot* data, bool rootState) override {
        auto insertedState = _storage.append(stateID, length, data, rootState);
        return insertedState.getState();
    }

    const llmc::storage::StorageInterface::FullState* getSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        auto ctx = static_cast<Context*>(ctx_);
        if constexpr(Storage::accessToStates()) {
            (void)ctx;
            return _storage.get(s, false);
        } else {
            size_t stateLength = _storage.determineLength(s);
            FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(llmc::storage::FullStateData<StateSlot>) / sizeof(StateSlot) + stateLength);
            FullState::create(dest, false, stateLength);
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
        return false;
    }

    llmc::storage::StorageInterface::Delta& newDelta(void* buffer, size_t offset, const StateSlot* data, size_t len) override {
        return llmc::storage::StorageInterface::Delta::create(buffer, offset, data, len);
    }

//    llmc::storage::StorageInterface::Delta* newDelta(size_t offset, StateSlot* data, size_t len) override {
//        return llmc::storage::StorageInterface::Delta::create(offset, data, len);
//    }

    void deleteDelta(llmc::storage::StorageInterface::Delta* d) override {
        return llmc::storage::StorageInterface::Delta::destroy(d);
    }

    llmc::storage::StorageInterface::StateTypeID newType(std::string const& name) override {
        static size_t t = 1;
        return t++;
    }
    bool setRootType(llmc::storage::StorageInterface::StateTypeID typeID) {
        _rootTypeID = typeID;
        return true;
    }

    Storage& getStorage() { return _storage; }

    size_t getStates() const {
        return _states;
    }

    size_t getTransitions() const {
        return _transitions;
    }

    void setSettings(Settings& settings) {
        _threads = settings["threads"];
        if(_threads == 0) {
            _threads = System::getNumberOfAvailableCores();
        }
    }

protected:
    mutex_type mtx;
    std::deque<StateID> stateQueueNew;
    size_t _states;
    size_t _transitions;
    StateTypeID _rootTypeID;
    Storage _storage;
    Listener& _listener;
    size_t _threads;
};