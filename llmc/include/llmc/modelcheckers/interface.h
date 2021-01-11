#pragma once

template<typename STORAGE>
class VModelChecker;

#include <vector>

#include <llmc/model.h>
#include <llmc/storage/interface.h>
#include <llmc/transitioninfo.h>

template<typename StateSlot>
class SimpleAllocator {
public:

    SimpleAllocator() {
        _buffer.reserve(1U << 24);
        _next = _buffer.data();
    }
    StateSlot* allocate(size_t slots) {
        auto current = _next;
        _next += (slots + (sizeof(uint64_t) / sizeof(StateSlot) - 1)) / (sizeof(uint64_t) / sizeof(StateSlot));
#ifndef NDEBUG
        if(_next > _buffer.data() + _buffer.capacity()) {
            fprintf(stderr, "SimpleAllocator(%p)::allocate(%zu): allocator full (%zu), next %p\n", this, slots, _buffer.capacity(), _next);
            fflush(stderr);
            abort();
        }
#endif
        return (StateSlot*)current;
    }

    size_t capacity() {
        return _buffer.capacity();
    }

    void clear() {
        _next = _buffer.data();
    }
private:
    uint64_t* _next;
    std::vector<uint64_t> _buffer;
};

//template<typename STATESLOT, typename STATETYPEID, typename STATEID, typename DELTA, typename MULTIDELTA, typename FULLSTATE>
//template<typename STORAGE>
//class ModelCheckerInterface: VModelChecker<STORAGE> {
//public:
//    using StateSlot = typename STORAGE::StateSlot;
//    using StateTypeID = typename STORAGE::StateTypeID;
//    using StateID = typename STORAGE::StateID;
//    using Delta = typename STORAGE::Delta;
//    using MultiDelta = typename STORAGE::MultiDelta;
//    using FullState = typename STORAGE::FullState;
//    using InsertedState = typename STORAGE::InsertedState;
//public:
//
//    template<typename MODEL>
//    struct ContextBase: public VContext<STORAGE> {
//        MODEL* model;
//
//        ContextBase(MODEL* model): model(model) {}
//
//        MODEL *getModel() const {
//            return model;
//        }
//
//    };
//
//    template<typename MODELCHECKER, typename MODEL>
//    struct ContextImpl: public ContextBase<MODEL> {
//        MODELCHECKER* mc;
//        StateID sourceState;
//
//        SimpleAllocator<StateSlot> allocator;
//
//        VModelChecker<STORAGE>* getModelChecker() {
//            return mc;
//        };
//        StateID getSourceState() const {
//            return sourceState;
//        }
//        ContextImpl(MODELCHECKER* mc_, MODEL* model): ContextBase<MODEL>(model), mc(mc_) {}
//    };
//
//    template<typename MODELCHECKER, typename MODEL>
//    using Context = ContextImpl<MODELCHECKER, MODEL>;
//public:
//
////    InsertedState newState(StateTypeID const& typeID, size_t length, StateSlot* slots);
////
////    template<typename TransitionInfoGetter>
////    StateID newTransition(Context* ctx, size_t length, StateSlot* slots, TransitionInfoGetter&& tInfo);
////
////    template<typename TransitionInfoGetter>
////    StateID newTransition(Context* ctx, MultiDelta const& delta, TransitionInfoGetter&& tInfo);
////
////    template<typename TransitionInfoGetter>
////    StateID newTransition(Context* ctx, Delta const& delta, TransitionInfoGetter&& tInfo);
////
////    const FullState* getState(Context* ctx, StateID const& s);
////    StateID newSubState(StateID const& stateID, Delta const& delta);
////    const FullState* getSubState(Context* ctx, StateID const& s);
////    Delta* newDelta(size_t offset, StateSlot* data, size_t len);
////    void deleteDelta(Delta* d);
////    bool newType(StateTypeID typeID, std::string const& name);
////    StateTypeID newType(std::string const& name);
////    bool setRootType(StateTypeID typeID);
//
//};
//
//using MCI = ModelCheckerInterface<StorageInterface>;

//template<typename MODEL, typename STORAGE>
//class ModelChecker: public ModelCheckerInterface<STORAGE> {
//public:
//
//    using StateSlot = typename STORAGE::StateSlot;
//    using StateTypeID = typename STORAGE::StateTypeID;
//    using StateID = typename STORAGE::StateID;
//    using Delta = typename STORAGE::Delta;
//    using MultiDelta = typename STORAGE::MultiDelta;
//    using FullState = typename STORAGE::FullState;
//    using Model = MODEL;
//
//    ModelChecker(Model* m): _m(m) {
//
//    }
//
//    Model* getModel() const { return _m; }
//
//protected:
//    Model* _m;
//};

struct VTransitionInfo {
    const char* _label;

    VTransitionInfo(const char* label) : _label(label) {};
};

template<typename STORAGE>
class VModelChecker {
public:
    using Storage = STORAGE;
    using StateSlot = typename STORAGE::StateSlot;
    using StateTypeID = typename STORAGE::StateTypeID;
    using StateID = typename STORAGE::StateID;
    using Delta = typename STORAGE::Delta;
    using MultiDelta = typename STORAGE::MultiDelta;
    using FullState = typename STORAGE::FullState;
    using InsertedState = typename STORAGE::InsertedState;

    virtual InsertedState newState(VContext<STORAGE>* ctx, StateTypeID const& typeID, size_t length, StateSlot* slots) = 0;
    virtual StateID newTransition(VContext<STORAGE>* ctx, size_t length, StateSlot* slots, TransitionInfoUnExpanded const& transition) = 0;
    virtual StateID newTransition(VContext<STORAGE>* ctx, MultiDelta const& delta, TransitionInfoUnExpanded const& transition) = 0;
    virtual StateID newTransition(VContext<STORAGE>* ctx, Delta const& delta, TransitionInfoUnExpanded const& transition) = 0;
    virtual StateID newTransition(VContext<STORAGE>* ctx, size_t offset, size_t length, const StateSlot* slots, TransitionInfoUnExpanded const& transition) = 0;
    virtual const FullState* getState(VContext<STORAGE>* ctx, StateID const& s) = 0;
    virtual StateID newSubState(VContext<STORAGE>* ctx, size_t length, StateSlot* slots) = 0;
    virtual StateID newSubState(VContext<STORAGE>* ctx, StateID const& stateID, Delta const& delta) = 0;
    virtual StateID newSubState(VContext<STORAGE>* ctx, StateID const& stateID, size_t offset, size_t length, const StateSlot* slots) = 0;
    virtual StateID appendState(VContext<STORAGE>* ctx, StateID const& stateID, size_t length, const StateSlot* slots, bool rootState) = 0;
    virtual const FullState* getSubState(VContext<STORAGE>* ctx, StateID const& s) = 0;
    virtual bool getStatePartial(VContext<STORAGE>* ctx, StateID const& s, size_t offset, StateSlot* data, size_t length, bool isRoot = true) = 0;
    virtual bool getSubStatePartial(VContext<STORAGE>* ctx, StateID const& s, size_t offset, StateSlot* data, size_t length) = 0;
    virtual Delta& newDelta(void* buffer, size_t offset, const StateSlot* data, size_t len) = 0;
    //virtual Delta* newDelta(size_t offset, StateSlot* data, size_t len) = 0;
    virtual void deleteDelta(Delta* d) = 0;
    virtual bool newType(StateTypeID typeID, std::string const& name) = 0;
    virtual StateTypeID newType(std::string const& name) = 0;
    virtual bool setRootType(StateTypeID typeID) = 0;

    VModelChecker(VModel<STORAGE>* m): _m(m) {

    }

    VModel<STORAGE>* getModel() const { return _m; }
//    STORAGE& getStorage() const = 0;

protected:
    VModel<STORAGE>* _m;
};