#include <iostream>

#include "interface.h"
#include "../../../../dtree/treedbs/include/treedbs/treedbs-ll.h"

template<typename HASHSET>
class TreeDBSStorage: public StorageInterface {
public:

    TreeDBSStorage(): _store(24) {

    }

    ~TreeDBSStorage() {
    }

    void init() {
        _store = TreeDBSLLcreate_dm(1024, 24, 2, nullptr, 0, 0, 0);
        std::cout << "Storage initialized with scale " << _store.getScale() << ", root scale " << _store.getRootScale() << std::endl;
    }

    void setScale(size_t scale) {
        _store.setScale(scale);
    }

    void setRootScale(size_t scale) {
        _store.setRootScale(scale);
    }

    using DTree = dtree<HASHSET>;
    using StateSlot = StorageInterface::StateSlot;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;
    struct StateID {
        StateID(int id): _id (id) {};
        int _id;
    };

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
    using InsertedState = typename DTree::IndexInserted;

    StateID find(FullState* state) {
        return TreeDBSLLfop(_store, state->getData(), false);
    }
    StateID find(StateSlot* state, size_t length, bool isRoot) {
        return TreeDBSLLfop(_store, state, false);
    }
    InsertedState insert(FullState* state) {
        auto idx = _store.insert(state->getData(), state->getLength(), state->isRoot());
        return idx;
    }
    InsertedState insert(StateSlot* state, size_t length, bool isRoot) {
        auto idx = _store.insert(state, length, isRoot);
        return idx;
    }
    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        auto idx = _store.delta(DTreeIndex(stateID.getData()), delta.getOffset(), delta.getData(), delta.getLength(), isRoot);
        return idx;
    }

    FullState* get(StateID id, bool isRoot) {
        auto fsd = FullState::create(isRoot, id.getLength());
        _store.get(id, fsd->getData(), isRoot);
        return fsd;
    }

    void printStats() {
    }

private:
    treedbs_ll_t _store;
};
