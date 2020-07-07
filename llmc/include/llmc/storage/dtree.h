#pragma once

#include <iostream>

#include "interface.h"
#include "../../../../dtree/dtree/include/dtree/dtree.h"
#include <libfrugi/Settings.h>

using namespace libfrugi;

namespace llmc::storage {

template<typename OUT, typename CONTAINER>
void printGraph(OUT& out, CONTAINER& elements, std::string const& colorCode) {

    unsigned char level[] = {' ', '_', '-', '+', '=', '*', '#'};

    size_t maxElements = 0;
    for(size_t element: elements) {
        maxElements = std::max(maxElements, element);
    }

    out << colorCode;
    for(size_t i = 0; i < elements.size(); ++i) {
        if(elements[i] == 0) {
            out << char(level[0]);
        } else if(elements[i] == maxElements) {
            out << char(level[6]);
        } else {
            size_t percent = 100 * elements[i] / maxElements;
            if(95 < percent) {
                out << char(level[5]);
            } else {
                out << char(level[1 + 3 * elements[i] / maxElements]);
            }
        }
    }
    out << "\033[0m";
    out << " " << colorCode << "#\033[0m = " << maxElements;
}

template<typename HASHSET>
class DTreeStorage : public StorageInterface {
public:

    DTreeStorage() : _store() {
        _store.setHandlerFull([this](uint64_t v, bool isRoot) {
            static std::atomic<bool> done = false;
            bool f = false;
            bool t = true;
            if(done.compare_exchange_strong(f, t)) {
                std::cerr << "FATAL ERROR: storage full when trying to insert " << v << "(" << isRoot << ")"
                          << std::endl;
                printStats();
                abort();
            }
        });
    }

    ~DTreeStorage() {
    }

    void init() {
        _store.init();
//        std::cout << "Storage initialized with scale " << _store.getScale() << ", root scale " << _store.getRootScale() << std::endl;
    }

    void thread_init() {
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
    using StateID = StorageInterface::StateID;
    using InsertedState = StorageInterface::InsertedState;
    using SparseOffset = StorageInterface::SparseOffset;

//    using StateID = typename dtree<HASHSET>::Index;
//    using InsertedState = typename dtree<HASHSET>::IndexInserted;

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

    StateID find(FullState* state) {
        auto idx = _store.find(state->getData(), state->getLength(), state->isRoot());
        return idx.getData();
    }

    StateID find(StateSlot* state, size_t length, bool isRoot) {
        auto idx = _store.find(state, length, isRoot);
        return idx.getData();
    }

    InsertedState insert(FullState* state) {
        auto idx = _store.insert(state->getData(), state->getLength(), state->isRoot());
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }

    InsertedState insert(StateSlot* state, size_t length, bool isRoot) {
        auto idx = _store.insert(state, length, isRoot);
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        auto idx = _store.deltaMayExtend(DTreeIndex(stateID.getData()), delta.getOffset(), delta.getData(), delta.getLength(),
                                isRoot);
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }
    InsertedState insert(StateID const& stateID, size_t offset, size_t length, const StateSlot* data, bool isRoot) {
        auto idx = _store.deltaMayExtend(DTreeIndex(stateID.getData()), offset, data, length, isRoot);
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }

    FullState* get(StateID id, bool isRoot) {
        DTreeIndex treeID(id.getData());
        FullState* dest = FullState::create(isRoot, determineLength(id));
        get(dest->getDataToModify(), id, isRoot);
        return dest;
    }

    bool get(StateSlot* dest, StateID id, bool isRoot) {
        return _store.get(id.getData(), (uint32_t*) dest, isRoot);
    }

    bool getPartial(StateID id, size_t offset, StateSlot* data, size_t length, bool isRoot) {
        return _store.getPartial(id.getData(), offset, length, data, isRoot);
    }

    bool getSparse(StateID id, uint32_t* buffer, uint32_t offsets, Projection offset, bool isRoot) {
        return _store.getSparse(id.getData(), buffer, offsets, offset.getOffsets(), isRoot);
    }

    InsertedState deltaSparse(StateID id, uint32_t* delta, uint32_t offsets, Projection projection, bool isRoot) {
        auto idx = _store.deltaSparse(id.getData(), delta, offsets, projection.getOffsets(), isRoot);
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }

    InsertedState deltaSparseToSingle(StateID id, uint32_t* delta, uint32_t offsets, Projection projection, bool isRoot) {
        SparseOffset* offset = projection.getOffsets();
        size_t startOffset = offset[0].getOffset();
        size_t singleLen = offset[offsets-1].getOffset() - startOffset + offset[offsets-1].getLength();

        StateSlot buffer[singleLen];

        getPartial(id, startOffset, buffer, singleLen, isRoot);

        SparseOffset* offsetEnd = offset + offsets;
        StateSlot* v = buffer - startOffset;
        while(offset < offsetEnd) {
            uint32_t o = offset->getOffset();
            uint32_t l = offset->getLength();
            memcpy(v + o, delta, l * sizeof(StateSlot));
            delta += l;
            offset++;
        }

        return insert(id, startOffset, singleLen, buffer, isRoot);

    }

    void printStats() {

        std::cout << "inserts (new):      " << _store.getProbeStats().insertsNew << std::endl;
        std::cout << "inserts (existing): " << _store.getProbeStats().insertsExisting << std::endl;
        std::cout << "finds:              " << _store.getProbeStats().finds << std::endl;
        std::cout << "probeCount:         " << _store.getProbeStats().probeCount;
        if((_store.getProbeStats().insertsNew + _store.getProbeStats().insertsExisting + _store.getProbeStats().finds) > 0) {
            std::cout << " (" << ((double)_store.getProbeStats().probeCount /
                        (_store.getProbeStats().insertsNew + _store.getProbeStats().insertsExisting +
                         _store.getProbeStats().finds))
                      << " per insert/find)" << std::endl;
        } else {
            std::cout << std::endl;
        }
        std::cout << "firstProbe:         " << _store.getProbeStats().firstProbe << std::endl;
        std::cout << "finalProbe:         " << _store.getProbeStats().finalProbe << std::endl;
        std::cout << "failedCAS:          " << _store.getProbeStats().failedCAS << std::endl;

        // Root map
        std::vector<size_t> density;
        density.reserve(128);
        auto mapRootStats = _store.getDensityStats(128, density, 0);
        std::cout << "root map:   " << std::endl;
        printGraph(std::cout, density, "\033[1;37;44m");

        std::cout << ", fill=" << ((double)100 * mapRootStats.bytesUsed / mapRootStats.bytesReserved) << "%" << std::endl;
        std::cout << "root hash map size in bytes: " << mapRootStats.bytesUsed << " / " << mapRootStats.bytesReserved << std::endl;

        // Data map
        density.clear();
        density.reserve(128);
        auto mapDataStats = _store.getDensityStats(128, density, 1);
        std::cout << "data map: " << std::endl;
        printGraph(std::cout, density, "\033[1;37;44m");

        std::cout << ", fill=" << ((double)100 * mapDataStats.bytesUsed / mapDataStats.bytesReserved) << "%" << std::endl;
        std::cout << "data hash map size in bytes: " << mapDataStats.bytesUsed << " / " << mapDataStats.bytesReserved << std::endl;

        std::unordered_map<size_t,size_t> allSizes;
        allSizes.reserve(128);
        _store.getAllSizes(allSizes);
        std::cout << "Size distribution of states:" << std::endl;
        for(auto& sz: allSizes) {
            std::cout << sz.first << ": " << sz.second << std::endl;
        }
    }

    Statistics getStatistics() {
        Statistics stats;
        auto mapStats = _store.getStats();
        stats._bytesReserved = mapStats.bytesReserved;
        stats._bytesInUse = mapStats.bytesUsed;
        stats._bytesMaximum = mapStats.bytesReserved;
        stats._elements = mapStats.elements;
        return stats;
    }

    static bool constexpr accessToStates() {
        return false;
    }

    static bool constexpr stateHasFixedLength() {
        return false;
    }

    static bool constexpr stateHasLengthInfo() {
        return false;
    }

    static bool constexpr needsThreadInit() {
        return false;
    }

    size_t determineLength(StateID const& s) const {
        size_t length;
        _store.getLength(s.getData(), length);
        return length;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 24;
    }

    void setSettings(Settings& settings) {
        size_t hashmapRootScale = settings["storage.dtree.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.dtree.hashmap_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapRootScale) {
            _store.setRootScale(hashmapRootScale);
        }

        size_t hashmapDataScale = settings["storage.dtree.hashmapdata_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.dtree.hashmap_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.hashmapdata_scale"].asUnsignedValue();
        hashmapDataScale = hashmapDataScale ? hashmapDataScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapDataScale) {
            _store.setDataScale(hashmapDataScale);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, DTreeStorage& tree) {
        out << "DTreeStorage["
            << "scale=" << tree._store.getScale()
            << "]"
            ;
        return out;
    }

    std::string getName() const {
        std::stringstream ss;
        ss << "dtree(" << _store.getRootScale() << "," << _store.getDataScale() << ")";
        return ss.str();
    }

private:
    dtree<HASHSET> _store;
};

} // namespace llmc::storage
