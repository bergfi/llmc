#pragma once

#include <iostream>
#include <llmc/common/tls.h>
#include <libfrugi/Settings.h>

#include "interface.h"

using namespace libfrugi;

extern "C" {
#include "../../../../dtree/treedbs/include/treedbs/treedbs-ll.h"
}

namespace llmc::storage {

template<typename VarLengthStorage>
class TreeDBSStorage : public StorageInterface {
public:

    TreeDBSStorage(): _stateLength(256), _hashmapDataScale(28), _hashmapRootScale(28), slim(0) {

    }

    ~TreeDBSStorage() {
    }

    void init() {
        _varLengthStorage.init();
        _store = TreeDBSLLcreate_dm(_stateLength, _hashmapRootScale, _hashmapRootScale - _hashmapDataScale, nullptr, 0, slim, 0);
        assert(_store);
        std::cout << "Storage initialized with state length " << _stateLength << std::endl;
        _scratchPadStorageLength = 16;
    }

    void thread_init() {
        if constexpr(VarLengthStorage::needsThreadInit()) {
            _varLengthStorage.thread_init();
        }
        _scratchPadStorage = (int*)malloc(_stateLength * _scratchPadStorageLength * 2 * sizeof(int));
        _scratchPadNext = _scratchPadStorage.get();
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

    StateID getIDFromTreeT(tree_t tree) {
        return TreeDBSLLindex(_store, tree) | (_stateLength << 40);
    }

    tree_ref_t getTreeRefFromID(StateID id) {
        return id.getData() & 0x000000FFFFFFFFFFULL;
    }

    StateID find(FullState* state) {
        return find((int*)state->getData(), state->getLength(), state->isRoot());
    }

    StateID find(StateSlot* state, size_t length, bool isRoot) {
        if(length != _stateLength || isRoot == false) {
            return _varLengthStorage.find(state, length, isRoot);
        }
        int t[_stateLength * 2];
        if(TreeDBSLLfop_incr(_store, (int*)state, nullptr, t, false) >= 0) {
            return getIDFromTreeT(t);
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
        if(length != _stateLength || isRoot == false) {
            return _varLengthStorage.insert(state, length, isRoot);
        }
        tree_t scratchPadForState = newScratchPadForState();
        auto seen = TreeDBSLLfop_incr(_store, (int*)state, NULL, scratchPadForState, true);
        return InsertedState(getIDFromTreeT(scratchPadForState), seen == 0);
    }

    tree_t findScratchPadForState(tree_ref_t treeRef) {
        tree_t pad = _scratchPadStorage.get();
        tree_t padEnd = pad + _scratchPadStorageLength * _stateLength * 2;
        while(pad < padEnd) {

            if(TreeDBSLLindex(_store, pad) == treeRef) {
                return pad;
            }

            pad = (tree_t)((uint32_t*)pad + _stateLength * 2);
        }
        return nullptr;
    }

    tree_t newScratchPadForState() {
        tree_t pad = _scratchPadStorage.get();
        assert(pad);
        tree_t padEnd = pad + _scratchPadStorageLength * _stateLength * 2;
        auto p = _scratchPadNext.get();
        auto p2 = (tree_t)((uint32_t*)p + _stateLength * 2);
        if(p2 >= padEnd) {
            p2 = pad;
        }
        _scratchPadNext = p2;
        assert(p);
        return p;
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        size_t length = determineLength(stateID);
        size_t newLength = std::max(length, delta.getOffset() + delta.getLength());
        if(newLength != _stateLength || isRoot == false) {
            return _varLengthStorage.insert(stateID, delta, isRoot);
        }
//        if(delta.getOffset() + delta.getLength() > getMaxStateLength()) {
//            fprintf(stderr, "Max state length exceeded: delta with length %zu and offset %zu\n", delta.getLength(), delta.getOffset());
//            abort();
//        }
        StateSlot v[newLength];
        tree_ref_t treeRef = getTreeRefFromID(stateID);
        tree_t prevScratchPad = findScratchPadForState(treeRef);
        tree_t newScratchPad = newScratchPadForState();
        if(length == _stateLength && prevScratchPad) {
            memcpy(v, prevScratchPad + _stateLength, sizeof(int) * _stateLength);
        } else {
            get(v, stateID, isRoot);
        }
        memcpy(v + delta.getOffset(), delta.getData(), delta.getLengthInBytes());
        if(delta.getOffset() > length) {
            memset(v+length, 0, (delta.getOffset() - length) * sizeof(StateSlot));
        }
        auto seen = TreeDBSLLfop_incr(_store, (int*)v, prevScratchPad, newScratchPad, true);
        return InsertedState(getIDFromTreeT(newScratchPad), seen == 0);
    }

    FullState* get(StateID stateID, bool isRoot) {
        assert(_stateLength);
        size_t length = determineLength(stateID);
        if(length != _stateLength || isRoot == false) {
            return _varLengthStorage.get(stateID, isRoot);
        }
        int d[_stateLength*2];
        TreeDBSLLget(_store, getTreeRefFromID(stateID), d);
        auto fsd = FullState::create(isRoot, _stateLength, (StateSlot*)d + _stateLength);
        return fsd;
    }

    bool get(StateSlot* dest, StateID stateID, bool isRoot) {
        assert(_stateLength);
        size_t length = determineLength(stateID);
        if(length != _stateLength || isRoot == false) {
            return _varLengthStorage.get(dest, stateID, isRoot);
        }
        int d[_stateLength*2];
        TreeDBSLLget(_store, getTreeRefFromID(stateID), d);
        memcpy(dest, (StateSlot*)d + _stateLength, _stateLength * sizeof(int));
        return true;
    }

    bool getPartial(StateID stateID, size_t offset, StateSlot* data, size_t length, bool isRoot) {
        assert(_stateLength);
        size_t len = determineLength(stateID);
        if(len != _stateLength || isRoot == false) {
            return _varLengthStorage.getPartial(stateID, offset, data, length, isRoot);
        }
        tree_ref_t treeRef = getTreeRefFromID(stateID);
        tree_t scratchPad = findScratchPadForState(treeRef);
        if(scratchPad) {
            memcpy(data, (StateSlot*)scratchPad + _stateLength + offset, length * sizeof(StateSlot));
            return true;
        }
        int d[_stateLength*2];
        TreeDBSLLget(_store, getTreeRefFromID(stateID), d);
        memcpy(data, (StateSlot*)d + _stateLength + offset, length * sizeof(StateSlot));
        return true;
    }

    void printStats() {
        auto stats = _varLengthStorage.getStatistics();
        std::cout << "Subcontainer uses " << stats.getBytesInUse()
                  << ", reserved " << stats.getBytesReserved()
                  << " for " << stats.getElements() << " elements"
                  << std::endl;
    }

    static bool constexpr accessToStates() {
        return false;
    }

    static bool constexpr stateHasFixedLength() {
        return true;
    }

    static bool constexpr stateHasLengthInfo() {
        return true;
    }

    size_t determineLength(StateID const& s) const {
        return (s.getData() & 0x7FFFFFFFFFFFFFFFULL) >> 40;
    }

    static bool constexpr needsThreadInit() {
        return true;
    }

    void setMaxStateLength(size_t stateLength) {
        _stateLength = stateLength;
    }

    size_t getMaxStateLength() const {
        return _stateLength;
    }

    void setSettings(Settings& settings) {
        size_t hashmapRootScale = settings["storage.treedbs.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.treedbs.hashmap_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapRootScale) {
            _hashmapRootScale = hashmapRootScale;
        }

        size_t hashmapDataScale = settings["storage.treedbs.hashmapdata_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.treedbs.hashmap_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.hashmapdata_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapDataScale) {
            _hashmapDataScale = hashmapDataScale;
        }

        slim = settings["storage.treedbs.slim"].asUnsignedValue();

        size_t fixedvectorsize = settings["storage.fixedvectorsize"].asUnsignedValue();
        if(fixedvectorsize) _stateLength = fixedvectorsize;

        _varLengthStorage.setSettings(settings);
    }

    Statistics getStatistics() {
        Statistics stats;
        TreeDBSLLstats2(_store, &stats._bytesInUse, &stats._bytesReserved);
        stats._bytesMaximum = stats._bytesReserved;

        stats += _varLengthStorage.getStatistics();

        return stats;
    }

    std::string getName() const {
        std::stringstream ss;
        ss << "treedbs<" << _varLengthStorage.getName() << ">(" << _hashmapRootScale << "," << _hashmapDataScale << ", " << slim << ")";
        return ss.str();
    }

private:
    VarLengthStorage _varLengthStorage;
    size_t _stateLength;
    treedbs_ll_t _store;
    llmc::TLS<int> _scratchPadStorage;
    llmc::TLS<int> _scratchPadNext;
    size_t _scratchPadStorageLength;
    size_t _hashmapDataScale;
    size_t _hashmapRootScale;
    size_t slim;
};

} // namespace llmc::storage