#pragma once

#include <llmc/storage/interface.h>
#include <llmc/model.h>

#include <vector>

template<typename StateSlot>
class SimpleAllocator {
public:

    SimpleAllocator() {
        _buffer.reserve(1U << 24);
        _next = _buffer.data();
    }
    StateSlot* allocate(size_t slots) {
        auto current = _next;
        _next += slots * sizeof(StateSlot) / sizeof(uint64_t);
        assert(_next <= _buffer.data() + _buffer.capacity());
        return (StateSlot*)current;
    }

    void clear() {
        _next = _buffer.data();
    }
private:
    uint64_t* _next;
    std::vector<uint64_t> _buffer;
};

//template<typename STATESLOT, typename STATETYPEID, typename STATEID, typename DELTA, typename MULTIDELTA, typename FULLSTATE>
template<typename STORAGE>
class ModelCheckerInterface {
public:
    using StateSlot = typename STORAGE::StateSlot;
    using StateTypeID = typename STORAGE::StateTypeID;
    using StateID = typename STORAGE::StateID;
    using Delta = typename STORAGE::Delta;
    using MultiDelta = typename STORAGE::MultiDelta;
    using FullState = typename STORAGE::FullState;
    using InsertedState = typename STORAGE::InsertedState;
public:

    template<typename MODEL>
    struct ContextBase {
        MODEL* model;
        StateID sourceState;

        SimpleAllocator<StateSlot> allocator;

        ContextBase(MODEL* model): model(model) {}

        MODEL *getModel() const {
            return model;
        }

        StateID getSourceState() const {
            return sourceState;
        }
    };

    template<typename MODELCHECKER, typename MODEL>
    struct ContextImpl: public ContextBase<MODEL> {
        MODELCHECKER* mc;

        SimpleAllocator<StateSlot> allocator;

        ContextImpl(MODELCHECKER* mc_, MODEL* model): ContextBase<MODEL>(model), mc(mc_) {}
    };

    using Context = ContextImpl<void, void>;
public:

    InsertedState newState(StateTypeID const& typeID, size_t length, StateSlot* slots);
    StateID newTransition(Context* ctx, size_t length, StateSlot* slots);
    StateID newTransition(Context* ctx, MultiDelta const& delta);
    StateID newTransition(Context* ctx, Delta const& delta);
    const FullState* getState(Context* ctx, StateID const& s);
    StateID newSubState(StateID const& stateID, Delta const& delta);
    const FullState* getSubState(Context* ctx, StateID const& s);
    Delta* newDelta(size_t offset, StateSlot* data, size_t len);
    void deleteDelta(Delta* d);
    bool newType(StateTypeID typeID, std::string const& name);
    StateTypeID newType(std::string const& name);
    bool setRootType(StateTypeID typeID);

};

using MCI = ModelCheckerInterface<StorageInterface>;

template<typename MODEL, typename STORAGE>
class ModelChecker: public ModelCheckerInterface<STORAGE> {
public:

    using StateSlot = typename STORAGE::StateSlot;
    using StateTypeID = typename STORAGE::StateTypeID;
    using StateID = typename STORAGE::StateID;
    using Delta = typename STORAGE::Delta;
    using MultiDelta = typename STORAGE::MultiDelta;
    using FullState = typename STORAGE::FullState;
    using Model = MODEL;

    ModelChecker(Model* m): _m(m) {

    }

    Model* getModel() const { return _m; }

protected:
    Model* _m;
};
