#include <iostream>

#include "interface.h"

extern "C" {
#include "../../../../dtree/treedbs/include/treedbs/treedbs-ll.h"
}

namespace llmc::storage {

class TreeDBSStorage : public StorageInterface {
public:

    TreeDBSStorage() {

    }

    ~TreeDBSStorage() {
    }

    void init() {
        _stateLength = 16;
        _store = TreeDBSLLcreate_dm(16, 24, 2, nullptr, 0, 0, 0);
//        std::cout << "Storage initialized with scale " << _store.getScale() << ", root scale " << _store.getRootScale() << std::endl;
    }

    void setScale(size_t scale) {
    }

    void setRootScale(size_t scale) {
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

//    struct StateID {
//        StateID(int id) : _id(id) {};
//        tree_ref_t _id;
//    };

//    class InsertedState {
//    public:
//
//        InsertedState(): _stateID{0}, _inserted(0) {}
//
//        InsertedState(StateID stateId, bool inserted): _stateID(stateId), _inserted(inserted) {}
//
//        StateID getState() const {
//            return _stateID;
//        }
//
//        bool isInserted() const {
//            return _inserted;
//        }
//    private:
//        StateID _stateID;
//        uint64_t _inserted;
//    };
//    using InsertedState = typename DTree::IndexInserted;

    StateID find(FullState* state) {
        tree_ref_t ref;
        if(TreeDBSLLfopZeroExtend_ref(_store, (int*)state->getData(), state->getLength(), state->isRoot(), false, &ref) >= 0) {
            return ref;
        } else {
            return StateID::NotFound();
        }
    }

    StateID find(StateSlot* state, size_t length, bool isRoot) {
        tree_ref_t ref;
        if(TreeDBSLLfopZeroExtend_ref(_store, (int*)state, length, isRoot, false, &ref) >= 0) {
            return ref;
        } else {
            return StateID::NotFound();
        }
    }

    InsertedState insert(FullState* state) {
//        tree_ref_t ref;
//        auto seen = TreeDBSLLfopZeroExtend_ref(_store, (int*)state->getData(), state->getLength(), state->isRoot(), true, &ref);
//        return InsertedState(ref, seen == 0);
        assert(state->getLength() < getMaxStateLength());
        return insert(state->getData(), state->getLength(), state->isRoot());
    }

    InsertedState insert(const StateSlot* state, size_t length, bool isRoot) {
        tree_ref_t ref;
        if(length > getMaxStateLength()) {
            fprintf(stderr, "Max state length exceeded: %zu\n", length);
            abort();
        }
        auto seen = TreeDBSLLfopZeroExtend_ref(_store, (int*)state, length, isRoot, true, &ref);
        return InsertedState(ref, seen == 0);
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
//        tree_ref_t ref;
//        int o[_stateLength*2 + 1];
//        int n[_stateLength*2 + 1];
//        o[_stateLength*2] = 0;
//        n[_stateLength*2] = 0;

        if(delta.getOffset() + delta.getLength() > getMaxStateLength()) {
            fprintf(stderr, "Max state length exceeded: delta with length %zu and offset %zu\n", delta.getLength(), delta.getOffset());
            abort();
        }
        int o[_stateLength*2];
        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), o, isRoot);
        memcpy(o+_stateLength+delta.getOffset(), delta.getData(), delta.getLengthInBytes());
        return insert((StateSlot*)o+_stateLength, stateID.getData() >> 40, isRoot);

//        int* o = (int*)malloc(_stateLength*4*sizeof(int));
//        int* n = o + _stateLength * 2;
//        fprintf(stderr, "_stateLength: %zu\n",_stateLength);
//        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), o, isRoot);
//        memcpy(n+_stateLength, o+_stateLength, _stateLength * sizeof(int));
//        memcpy(n+_stateLength+delta.getOffset(), delta.getData(), delta.getLengthInBytes());
//        auto seen = TreeDBSLLfop_incr_ref(_store, o, n, isRoot, true, &ref);
//        free(o);
//        return InsertedState(ref, seen == 0);

        //        tree_ref_t ref;
//        auto seen = TreeDBSLLfopZeroExtend_ref(_store, state, length, true, &ref);
//        return InsertedState(ref, seen > 0);
    }

    FullState* get(StateID id, bool isRoot) {
        int d[_stateLength*2];
        TreeDBSLLget_isroot(_store, (tree_ref_t)id.getData(), d, isRoot);
        int length = id.getData() >> 40;
        auto fsd = FullState::create(isRoot, length, (StateSlot*)d + _stateLength);
        return fsd;
    }

    bool get(StateSlot* dest, StateID id, bool isRoot) {
        int d[_stateLength*2];
        TreeDBSLLget_isroot(_store, (tree_ref_t)id.getData(), d, isRoot);
        int length = id.getData() >> 40;
        memcpy(dest, d + _stateLength, length * sizeof(int));
        return true;
    }

    void printStats() {
    }

    static bool constexpr accessToStates() {
        return false;
    }

    static bool constexpr stateHasFixedLength() {
        return false;
    }

    static bool constexpr stateHasLengthInfo() {
        return true;
    }

    static bool constexpr needsThreadInit() {
        return false;
    }

    size_t getMaxStateLength() const {
        return _stateLength;
    }

    size_t determineLength(StateID const& s) const {
        return s.getData() >> 40;
    }

private:
    size_t _stateLength;
    treedbs_ll_t _store;
};

} // namespace llmc::storage