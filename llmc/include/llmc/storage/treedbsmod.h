#pragma once

#include <iostream>

#include "interface.h"
#include <libfrugi/Settings.h>

using namespace libfrugi;

extern "C" {
#include "../../../../dtree/treedbs/include/treedbs/treedbs-ll.h"
}

namespace llmc::storage {

class TreeDBSStorageModified : public StorageInterface {
public:

    TreeDBSStorageModified(): _stateLength(256), _hashmapDataScale(28), _hashmapRootScale(28) {

    }

    ~TreeDBSStorageModified() {
    }

    void init() {
        zeroRoots.clear();
        zeroRoots.reserve(16);
        _store = TreeDBSLLcreate_dm(_stateLength, _hashmapRootScale, _hashmapRootScale - _hashmapDataScale, nullptr, 0, 0, 0);
        assert(_store);
        std::cout << "Storage initialized with state length " << _stateLength << std::endl;
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

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
        assert(state->getLength() < getMaxStateLength());
        return insert(state->getData(), state->getLength(), state->isRoot());
    }

    InsertedState insert(const StateSlot* state, size_t length, bool isRoot) {
        tree_ref_t ref;
        if(length > getMaxStateLength()) {
            fprintf(stderr, "Max state length exceeded: %zu < %zu\n", getMaxStateLength(), length);
            abort();
        }
        auto seen = TreeDBSLLfopZeroExtend_ref(_store, (int*)state, length, isRoot, true, &ref);
        if(ref == 0xFFFFFFFFFFFFFFFFULL) abort();
        return InsertedState(ref, seen == 0);
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        return insert(stateID, delta.getOffset(), delta.getLength(), delta.getData(), isRoot);
    }
    InsertedState insert(StateID const& stateID, size_t offset, size_t length, const StateSlot* data, bool isRoot) {

        size_t oldLength = stateID.getData() >> 40;

        size_t newLength = std::max(offset + length, oldLength);

        if(newLength > getMaxStateLength()) {
            fprintf(stderr, "Max state length (%zu) exceeded: delta with length %zu and offset %zu\n", getMaxStateLength(), length, offset);
            abort();
        }

        int o[_stateLength*2];
        int n[_stateLength*2];
        if(stateID.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        memset(o, 0, sizeof(o));
        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), o, isRoot);
        o[1] &= 0x00FFFFFF;

        memcpy(n, o, sizeof(o));
        memcpy(n+_stateLength+offset, data, length * sizeof(StateSlot));

        tree_ref_t ref;
        auto seen = TreeDBSLLfop_incr_ref(_store, o, n, isRoot, true, &ref);
        ref &= 0x000000FFFFFFFFFFULL;
        ref |= newLength << 40;

        return InsertedState(ref, seen == 0);


    }

    FullState* get(StateID id, bool isRoot) {
        int d[_stateLength*2];
        if(id.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        TreeDBSLLget_isroot(_store, (tree_ref_t)id.getData(), d, isRoot);
        int length = id.getData() >> 40;
        auto fsd = FullState::create(isRoot, length, (StateSlot*)d + _stateLength);
        return fsd;
    }

    bool get(StateSlot* dest, StateID id, bool isRoot) {
        int d[_stateLength*2];
        if(id.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        TreeDBSLLget_isroot(_store, (tree_ref_t)id.getData(), d, isRoot);
        int length = id.getData() >> 40;
        memcpy(dest, d + _stateLength, length * sizeof(int));
        return true;
    }

    bool getPartial(StateID stateID, size_t offset, StateSlot* data, size_t length, bool isRoot) {
        int d[_stateLength*2];
        if(stateID.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), d, isRoot);
        memcpy(data, (StateSlot*)d + _stateLength + offset, length * sizeof(StateSlot));
        return true;
    }

    void getPartial(StateID id, MultiProjection& projection, bool isRoot, uint32_t* buffer) {
        size_t length = 0;
        for(size_t p = 0; p < projection.getProjections(); ++p) {
            length += projection.getProjection(p).getLengthAndOffsets().getLength();
        }
        memset(buffer, 0, length * sizeof(uint32_t));
    }
    InsertedState delta(StateID idx, MultiProjection& projection, bool isRoot, uint32_t* buffer) {
        return InsertedState();
    }

    bool getSparse(StateID stateID, uint32_t* buffer, uint32_t offsets, SparseOffset* offset, bool isRoot) {
        int d[_stateLength*2];
        if(stateID.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), d, isRoot);
        SparseOffset* offsetEnd = offset + offsets;
        while(offset < offsetEnd) {
            uint32_t o = offset->getOffset();
            uint32_t l = offset->getLength();
            memcpy(buffer, (StateSlot*) d + _stateLength + o, l * sizeof(StateSlot));
            buffer += l;
            offset++;
        }
        return true;
    }

    InsertedState deltaSparse(StateID stateID, uint32_t* delta, uint32_t offsets, SparseOffset* offset, bool isRoot) {
        size_t length = stateID.getData() >> 40;

        int oldV[_stateLength*2];
        int newV[_stateLength*2];
        if(stateID.getData() == 0xFFFFFFFFFFFFFFFFULL) abort();
        oldV[0] = 0;
        TreeDBSLLget_isroot(_store, (tree_ref_t)stateID.getData(), oldV, isRoot);
        oldV[1] &= 0x00FFFFFF;

        memcpy(newV, oldV, sizeof(oldV));
        SparseOffset* offsetEnd = offset + offsets;
        int* v = newV + _stateLength;
        while(offset < offsetEnd) {
            uint32_t o = offset->getOffset();
            uint32_t l = offset->getLength();
            memcpy(v + o, delta, l * sizeof(StateSlot));
            delta += l;
            offset++;
        }

        tree_ref_t ref;
        auto seen = TreeDBSLLfop_incr_ref(_store, oldV, newV, isRoot, true, &ref);
        ref &= 0x000000FFFFFFFFFFULL;
        ref |= length << 40;

        return InsertedState(ref, seen == 0);

    }

    void printStats() {
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

    static bool constexpr needsThreadInit() {
        return false;
    }

    void setMaxStateLength(size_t stateLength) {
         _stateLength = stateLength;
    }

    size_t getMaxStateLength() const {
        return _stateLength;
    }

    size_t determineLength(StateID const& s) const {
        return s.getData() >> 40;
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
        size_t fixedvectorsize = settings["storage.fixedvectorsize"].asUnsignedValue();
        if(fixedvectorsize) _stateLength = fixedvectorsize;
    }

    Statistics getStatistics() {
        Statistics stats;
        TreeDBSLLstats2(_store, &stats._bytesInUse, &stats._bytesReserved);
        stats._bytesMaximum = stats._bytesReserved;
        return stats;
    }

    std::string getName() const {
        std::stringstream ss;
        ss << "treedbsmod(" << _hashmapRootScale << "," << _hashmapDataScale << ")";
        return ss.str();
    }

private:
    size_t _stateLength;
    treedbs_ll_t _store;
    std::vector<size_t> zeroRoots;
    size_t _hashmapDataScale;
    size_t _hashmapRootScale;
};

} // namespace llmc::storage