#pragma once

#include <deque>
#include <libfrugi/System.h>
#include <libfrugi/Settings.h>

#include <llmc/statespace/listener.h>

using namespace libfrugi;

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
        size_t stateBytesInsertedIntoStorage;
        Context(VModelChecker<llmc::storage::StorageInterface>* mc, VModel<llmc::storage::StorageInterface>* model): VContext<llmc::storage::StorageInterface>(mc, model), stateBytesInsertedIntoStorage(0) {}
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
        if constexpr(Storage::needsThreadInit()) {
            _storage.thread_init();
        }

        Context ctx(this, this->_m);
        _m->init(&ctx);
        if(_m->getStateVectorType()) {
            std::cout << "State vector is of type " << *_m->getStateVectorType() << std::endl;
        } else {
            std::cout << "State vector has no type" << std::endl;
        }
        StateID init = this->_m->getInitial(&ctx);

        printf("Initial state is %zu long\n", _storage.determineLength(init));

        System::Timer timer;
//        printf("modelchecker says ctx:%p model:%p\n", &ctx, ctx.model);
        if(init.exists()) {
//            auto fsd = getState(&ctx, init);
//            _listener.writeState(this->getModel(), init, fsd->getLength(), fsd->getData());
//            _listener.writeState(this->getModel(), init, 0, nullptr);
//            stateQueueNew.emplace_back(init);
            int level = 0;

//            _states++;
//            ctx.stateBytesInsertedIntoStorage += _storage.determineLength(init);

//            size_t statesLastCheck = _states;

            do {
//                printf("level %i has %i states\n", level, _states);
                level++;
                int level_states = 0;
                stateQueueNew.swap(stateQueue);
                do {
                    StateID s = stateQueue.front();
                    stateQueue.pop_front();
                    ctx.sourceState = s;
                    ctx.allocator.clear();
//                    printf("=== %zu\n", s);
                    size_t oldNr = _transitions;
                    this->_m->getNextAll(ctx.sourceState, &ctx);
                    if(oldNr == _transitions) {

                        // TODO: dirty hack, this checks the PC of the first process
                        uint32_t status;
                        getStatePartial(&ctx, ctx.sourceState.getData(), 6, &status, 1);
                        if(status != 0) {
                            printf("INVALID END STATE %zu\n", ctx.sourceState.getData());
                        } else {
                            printf("VALID END STATE %zu\n", ctx.sourceState.getData());
                        }
                    }
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

        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states, _transitions);
        printf("States/s: %lf\n", ((double)_states)/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Storage Stats (%s):\n", _storage.getName().c_str());
        //Stats
        auto stats = _storage.getStatistics();
        _storage.printStats();
        printf("Inserted vector-data in bytes: %zu\n", ctx.stateBytesInsertedIntoStorage);
        printf("Compression ratio (%zu used bytes): %lf (%lf bytes per state)\n", stats._bytesInUse, (double)ctx.stateBytesInsertedIntoStorage / (stats._bytesInUse), (double)stats._bytesInUse / _states);
        printf("Compression ratio (%zu reserved bytes): %lf (%lf bytes per state)\n", stats._bytesReserved, (double)ctx.stateBytesInsertedIntoStorage / (stats._bytesReserved), (double)stats._bytesReserved / _states);
        _listener.finish();
    }

//    void goDFS() {
//        _listener.init();
//        _storage.init();
//        if constexpr(Storage::needsThreadInit()) {
//            _storage.thread_init();
//        }
//
//        Context ctx(this, this->_m);
//        _m->init(&ctx);
//        if(_m->getStateVectorType()) {
//            std::cout << "State vector is of type" << *_m->getStateVectorType() << std::endl;
//        } else {
//            std::cout << "State vector has no type" << std::endl;
//        }
//        StateID init = this->_m->getInitial(&ctx);
//
//        printf("Initial state is %zu long\n", _storage.determineLength(init));
//
//        System::Timer timer;
//        if(init.exists()) {
//            stateQueueNew.emplace_back(init);
//            int level = 0;
//
//            _states++;
//            size_t statesLastCheck = _states;
//
//            do {
//                StateID s = stateQueueNew.back();
//                stateQueueNew.pop_back();
//                ctx.sourceState = s;
//                ctx.allocator.clear();
//
//                for
//                    this->_m->getNext(ctx.sourceState, &ctx, tg);
//            } while(!stateQueueNew.empty());
//        }
//        done:
//        auto elapsed = timer.getElapsedSeconds();
//        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
//        printf("Found %zu states, explored %zu transitions\n", _states, _transitions);
//        printf("States/s: %lf\n", ((double)_states)/elapsed);
//        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
//        printf("Storage Stats:\n");
//        _storage.printStats();
//        _listener.finish();
//    }

    llmc::storage::StorageInterface::InsertedState newState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateTypeID const& typeID, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        Context *ctx = static_cast<Context *>(ctx_);
        auto insertedState = _storage.insert(slots, length, typeID == _rootTypeID); // could make it 0?

        // Need to up state count, but newState is currently used for chunks as well
        //TODO ctx->stateBytesInsertedIntoStorage += _storage.determineLength(insertedState.getState());

        if(insertedState.isInserted()) {
//            printf("new transition %16zu -> %16u\n", stateID, insertedState.getState());
            _states++;
            auto fsd = FullState::createExternal(true, length, slots);
            _listener.writeState(this->getModel(), insertedState.getState(), fsd);
            fsd->destroy();
//            _listener.writeState(this->getModel(), insertedState.getState(), 0, nullptr);
            stateQueueNew.push_back(insertedState.getState());
            ctx->stateBytesInsertedIntoStorage += _storage.determineLength(insertedState.getState()) * sizeof(StateSlot);
        }
        auto r = llmc::storage::StorageInterface::InsertedState(insertedState.getState(), insertedState.isInserted());
//        std::cout << "[SCM] newState(" << typeID << ", " << length << ") -> " << r << std::endl;
        assert(r.getState().getData());
        return r;
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        StateID const& stateID = ctx->sourceState;
        // the type ID could be extracted from stateID...
        auto insertedState = newState(ctx, _rootTypeID, length, slots);
        _listener.writeTransition(stateID, insertedState.getState(), _m->getTransitionInfo(ctx, tinfo));
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::MultiDelta const& delta, TransitionInfoUnExpanded const& tinfo) override {
//        Context *ctx = static_cast<Context *>(ctx_);
        abort();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::Delta const& delta, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, delta, true);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
            auto fsd = getState(ctx_, insertedState.getState());
            _listener.writeState(this->getModel(), insertedState.getState(), fsd);
//            _listener.writeState(this->getModel(), insertedState.getState(), 0, nullptr);
            ctx->stateBytesInsertedIntoStorage += _storage.determineLength(insertedState.getState()) * sizeof(StateSlot);
        }
        _listener.writeTransition(stateID, insertedState.getState(), _m->getTransitionInfo(ctx, tinfo));
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newTransition(VContext<llmc::storage::StorageInterface>* ctx_, size_t offset, size_t length, const StateSlot* slots, TransitionInfoUnExpanded const& tinfo) override {
        Context *ctx = static_cast<Context *>(ctx_);
        StateID const& stateID = ctx->sourceState;
        auto insertedState = _storage.insert(stateID, offset, length, slots, true);
        if(insertedState.isInserted()) {
            _states++;
            stateQueueNew.push_back(insertedState.getState());
            auto fsd = getState(ctx_, insertedState.getState());
            _listener.writeState(this->getModel(), insertedState.getState(), fsd);
//            _listener.writeState(this->getModel(), insertedState.getState(), 0, nullptr);
            ctx->stateBytesInsertedIntoStorage += _storage.determineLength(insertedState.getState()) * sizeof(StateSlot);
        }
        _listener.writeTransition(stateID, insertedState.getState(), _m->getTransitionInfo(ctx, tinfo));
        _transitions++;
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::FullState* getState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context *ctx = static_cast<Context *>(ctx_);
        if constexpr(Storage::accessToStates()) {
            (void)ctx;
            return _storage.get(s, true);
        } else {
            size_t stateLength = _storage.determineLength(s);
            assert(stateLength && "getting state with length 0");
//            printf("Allocating %zu\n", sizeof(llmc::storage::FullStateData<StateSlot>) / sizeof(StateSlot) + stateLength);
            llmc::storage::StorageInterface::FullState* dest = (FullState*)ctx->allocator.allocate(sizeof(llmc::storage::FullStateData<StateSlot>) / sizeof(StateSlot) + stateLength);
            llmc::storage::StorageInterface::FullState::create(dest, true, stateLength);
            _storage.get(dest->getDataToModify(), s, true);
            return dest;
        }
    }

    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, size_t length, llmc::storage::StorageInterface::StateSlot* slots) override {
        Context *ctx = static_cast<Context *>(ctx_);
        auto insertedState = _storage.insert(slots, length, false);
        assert(length);
        assert(insertedState.getState().getData());
        auto fsd = getSubState(ctx_, insertedState.getState());
        _listener.writeState(this->getModel(), insertedState.getState(), fsd);
        assert((insertedState.getState().getData() >> 40) && "tried to return chunkID without length");
//        printf("newSubState: %zx\n", insertedState.getState());

        if(insertedState.isInserted()) {
            ctx->stateBytesInsertedIntoStorage += length * sizeof(StateSlot);
        }
        return insertedState.getState();
    }
    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, Delta const& delta) override {
        Context *ctx = static_cast<Context *>(ctx_);
        auto insertedState = _storage.insert(stateID, delta, false);
        assert(insertedState.getState().getData());
        auto fsd = getSubState(ctx_, insertedState.getState());
        _listener.writeState(this->getModel(), insertedState.getState(), fsd);

        if(insertedState.isInserted()) {
            ctx->stateBytesInsertedIntoStorage += fsd->getLength() * sizeof(StateSlot);
        }
//        auto fsdOld = getSubState(ctx_, insertedState.getState());
//        printf("[CAM64] before: %zx %u, delta: %u %u, after: %zx %u\n", stateID, fsdOld->getLength(), delta.getOffset(), delta.getLength(), insertedState.getState(), fsd->getLength());

//        printf("newSubState: %zx\n", insertedState.getState());
        assert((insertedState.getState().getData() >> 40) && "tried to return chunkID without length");
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID newSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, size_t offset, size_t length, const StateSlot* data) override {
        Context *ctx = static_cast<Context *>(ctx_);
        auto insertedState = _storage.insert(stateID, offset, length, data, false);
        assert(insertedState.getState().getData());
        auto fsd = getSubState(ctx_, insertedState.getState());
        _listener.writeState(this->getModel(), insertedState.getState(), fsd);

        if(insertedState.isInserted()) {
            ctx->stateBytesInsertedIntoStorage += fsd->getLength() * sizeof(StateSlot);
        }

        //        auto fsdOld = getSubState(ctx_, insertedState.getState());
//        printf("[CAM64] before: %zx %u, delta: %u %u, after: %zx %u\n", stateID, fsdOld->getLength(), delta.getOffset(), delta.getLength(), insertedState.getState(), fsd->getLength());

//        printf("newSubState: %zx\n", insertedState.getState());
        assert((insertedState.getState().getData() >> 40) && "tried to return chunkID without length");
        return insertedState.getState();
    }

    llmc::storage::StorageInterface::StateID appendState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& stateID, size_t length, const StateSlot* data, bool rootState) override {
        Context *ctx = static_cast<Context *>(ctx_);

        assert(!rootState && "need to do root states like we do in newTransition");

        auto insertedState = _storage.append(stateID, length, data, rootState);
        assert(insertedState.getState().getData());
        auto fsd = getSubState(ctx_, insertedState.getState());
        _listener.writeState(this->getModel(), insertedState.getState(), fsd);

        if(insertedState.isInserted()) {
            ctx->stateBytesInsertedIntoStorage += fsd->getLength() * sizeof(StateSlot);
        }
//        auto fsdOld = getSubState(ctx_, insertedState.getState());
//        printf("[CAM64] before: %zx %u, delta: %u %u, after: %zx %u\n", stateID, fsdOld->getLength(), delta.getOffset(), delta.getLength(), insertedState.getState(), fsd->getLength());

//        printf("newSubState: %zx\n", insertedState.getState());
        assert((insertedState.getState().getData() >> 40) && "tried to return chunkID without length");
        return insertedState.getState();
    }

    const FullState* getSubState(VContext<llmc::storage::StorageInterface>* ctx_, llmc::storage::StorageInterface::StateID const& s) override {
        Context *ctx = static_cast<Context *>(ctx_);
        if constexpr(Storage::accessToStates()) {
            (void)ctx;
            return _storage.get(s, false);
        } else {
            size_t stateLength = _storage.determineLength(s);
            assert(stateLength);
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

    STORAGE const& getStorage() const { return _storage; }
    STORAGE& getStorage() { return _storage; }

    size_t getStates() const {
        return _states;
    }

    size_t getTransitions() const {
        return _transitions;
    }

    void setSettings(Settings& settings) {
        _listener.setSettings(settings);
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