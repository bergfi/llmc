//#pragma once
//
//#include <iostream>
//
//#include "interface.h"
//#include "../../../../dtree/dtree/include/dtree/dtree.h"
//#include "dtree.h"
//
//namespace llmc::storage {
//
//template<typename ROOTHASHSET, typename HASHSET>
//class DTree2Storage : public StorageInterface {
//public:
//
//    DTree2Storage() : _rootStore(24), _store(24) {
//
//    }
//
//    ~DTree2Storage() {
//    }
//
//    void init() {
//        _store.init();
////        std::cout << "Storage initialized with scale " << _store.getScale() << ", root scale " << _store.getRootScale() << std::endl;
//    }
//
//    void thread_init() {
//    }
//
//    void setScale(size_t scale) {
//        _store.setScale(scale);
//    }
//
//    void setRootScale(size_t scale) {
//        _store.setRootScale(scale);
//    }
//
//    using DTree = dtree<HASHSET>;
//    using StateSlot = StorageInterface::StateSlot;
//    using StateTypeID = StorageInterface::StateTypeID;
//    using Delta = StorageInterface::Delta;
//    using MultiDelta = StorageInterface::MultiDelta;
//    using StateID = StorageInterface::StateID;
//    using InsertedState = StorageInterface::InsertedState;
////    using StateID = typename dtree<HASHSET>::Index;
////    using InsertedState = typename dtree<HASHSET>::IndexInserted;
//
////    class InsertedState {
////    public:
////
////        InsertedState(): _stateID{0}, _inserted(0) {}
////
////        InsertedState(StateID stateId, bool inserted): _stateID(stateId), _inserted(inserted) {}
////
////        StateID getState() const {
////            return _stateID;
////        }
////
////        bool isInserted() const {
////            return _inserted;
////        }
////    private:
////        StateID _stateID;
////        uint64_t _inserted;
////    };
//
//    StateID find(FullState* state) {
//        return find(state->getData(), state->getLength(), state->isRoot());
//    }
//
//    template<int isRoot = 0>
//    StateID find(StateSlot* state, size_t length) {
//        auto idx = _store.find(state, length, false);
//        if(idx == HASHSET::NotFound()) {
//            return StateID::NotFound();
//        } else if(isRoot) {
//            return _rootStore.find(idx.getState().getData(), length);
//        } else {
//            return StateID::NotFound();
//        }
//    }
//
//    StateID find(StateSlot* state, size_t length, bool isRoot) {
//        auto idx = _store.find(state, length, false);
//        if(idx == HASHSET::NotFound()) {
//            return StateID::NotFound();
//        } else if(isRoot) {
//            return _rootStore.find(idx.getState().getData(), length);
//        } else {
//            return StateID::NotFound();
//        }
//    }
//
//    InsertedState insert(FullState* state) {
//        return insert(state->getData(), state->getLength(), state->isRoot());
//    }
//
//    template<int isRoot = 0>
//    InsertedState insert(StateSlot* state, size_t length) {
//        auto idx = _store.insert(state, length, false);
//        if(isRoot) {
//            auto idx2 = _rootStore.insert(idx.getState().getData(), length);
//            return InsertedState(ROOTHASHSET::toID(idx2), ROOTHASHSET::toIsInserted(idx2));
//        } else {
//            return idx;
//        }
//    }
//
//    InsertedState insert(StateSlot* state, size_t length, bool isRoot) {
//        auto idx = _store.insert(state, length, false);
//        if(isRoot) {
//            auto idx2 = _rootStore.insert(idx.getState().getData(), length);
//            return InsertedState(ROOTHASHSET::toID(idx2), ROOTHASHSET::toIsInserted(idx2));
//        } else {
//            return idx;
//        }
//    }
//
//    template<int isRoot = 0>
//    InsertedState insert(StateID const& stateID, Delta const& delta) {
//        auto idx = _store.delta(DTreeIndex(stateID.getData()), delta.getOffset(), delta.getData(), delta.getLength(), isRoot);
//        if(isRoot) {
//            auto idx2 = _rootStore.insert(idx.getState().getData(), idx.getState().getData());
//            return InsertedState(ROOTHASHSET::toID(idx2), ROOTHASHSET::toIsInserted(idx2));
//        } else {
//            return idx;
//        }
//    }
//
//    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
//    }
//
//    template<int isRoot = 0>
//    FullState* get(StateID id) {
//        DTreeIndex treeID(id.getData());
//        FullState* dest = FullState::create(true, treeID.getLength());
//        get<isRoot>(dest->getDataToModify(), id);
//        return dest;
//    }
//
//    FullState* get(StateID id, bool isRoot) {
//        DTreeIndex treeID(id.getData());
//        FullState* dest = FullState::create(true, treeID.getLength());
//        get<isRoot>(dest->getDataToModify(), id);
//        return dest;
//    }
//
//    template<int isRoot = 0>
//    bool get(StateSlot* dest, StateID id) {
//        if(isRoot) {
//            return _rootStore.get(id.getData(), (uint32_t*) dest, isRoot);
//        } else {
//            return _store.get(id.getData(), (uint32_t*) dest, isRoot);
//        }
//    }
//
//    bool get(StateSlot* dest, StateID id, bool isRoot) {
//        if(isRoot) {
//            return get<1>(dest, id);
//        } else {
//            return get<0>(dest, id);
//        }
//    }
//
//    void printStats() {
//        std::cout << "inserts (new):      " << _store.getProbeStats().insertsNew << std::endl;
//        std::cout << "inserts (existing): " << _store.getProbeStats().insertsExisting << std::endl;
//        std::cout << "finds:              " << _store.getProbeStats().finds << std::endl;
//        std::cout << "probeCount:         " << _store.getProbeStats().probeCount
//                  << " (" << (_store.getProbeStats().probeCount /
//                              (_store.getProbeStats().insertsNew + _store.getProbeStats().insertsExisting +
//                               _store.getProbeStats().finds))
//                  << " per insert/find)" << std::endl;
//        std::cout << "firstProbe:         " << _store.getProbeStats().firstProbe << std::endl;
//        std::cout << "finalProbe:         " << _store.getProbeStats().finalProbe << std::endl;
//        std::cout << "failedCAS:          " << _store.getProbeStats().failedCAS << std::endl;
//        std::cout << "max size:           " << (1 << _store.getScale()) << std::endl;
//        std::vector<size_t> density;
//        density.reserve(128);
//        _store.getDensityStats(128, density, 0);
//        std::cout << "root map:   " << std::endl;
//        printGraph(std::cout, density, "\033[1;37;44m");
//        std::cout << std::endl;
//        density.clear();
//        density.reserve(128);
//        _store.getDensityStats(128, density, 1);
//        std::cout << "common map: " << std::endl;
//        printGraph(std::cout, density, "\033[1;37;44m");
//        std::cout << std::endl;
//    }
//
//    static bool constexpr accessToStates() {
//        return false;
//    }
//
//    static bool constexpr stateHasFixedLength() {
//        return false;
//    }
//
//    static bool constexpr stateHasLengthInfo() {
//        return true;
//    }
//
//    static bool constexpr needsThreadInit() {
//        return false;
//    }
//
//    size_t determineLength(StateID const& s) const {
//        return ((typename dtree<HASHSET>::Index*) &s)->getLength();;
//    }
//
//    size_t getMaxStateLength() const {
//        return 1ULL << 24;
//    }
//
//private:
//    ROOTHASHSET _rootStore;
//    dtree<HASHSET> _store;
//};
//
//} // namespace llmc::storage
