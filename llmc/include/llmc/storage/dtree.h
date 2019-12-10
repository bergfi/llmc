#include <iostream>

#include "interface.h"
#include "../../../../dtree/dtree/include/dtree/dtree.h"

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

    DTreeStorage() : _store(24) {

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
        auto idx = _store.delta(DTreeIndex(stateID.getData()), delta.getOffset(), delta.getData(), delta.getLength(),
                                isRoot);
        return InsertedState(idx.getState().getData(), idx.isInserted());
    }

    FullState* get(StateID id, bool isRoot) {
        DTreeIndex treeID(id.getData());
        FullState* dest = FullState::create(true, treeID.getLength());
        get(dest->getDataToModify(), id, isRoot);
        return dest;
    }

    bool get(StateSlot* dest, StateID id, bool isRoot) {
        return _store.get(id.getData(), (uint32_t*) dest, isRoot);
    }

    void printStats() {
        std::cout << "inserts (new):      " << _store.getProbeStats().insertsNew << std::endl;
        std::cout << "inserts (existing): " << _store.getProbeStats().insertsExisting << std::endl;
        std::cout << "finds:              " << _store.getProbeStats().finds << std::endl;
        std::cout << "probeCount:         " << _store.getProbeStats().probeCount
                  << " (" << (_store.getProbeStats().probeCount /
                              (_store.getProbeStats().insertsNew + _store.getProbeStats().insertsExisting +
                               _store.getProbeStats().finds))
                  << " per insert/find)" << std::endl;
        std::cout << "firstProbe:         " << _store.getProbeStats().firstProbe << std::endl;
        std::cout << "finalProbe:         " << _store.getProbeStats().finalProbe << std::endl;
        std::cout << "failedCAS:          " << _store.getProbeStats().failedCAS << std::endl;
        std::cout << "max size:           " << (1 << _store.getScale()) << std::endl;
        std::vector<size_t> density;
        density.reserve(128);
        _store.getDensityStats(128, density, 0);
        std::cout << "root map:   " << std::endl;
        printGraph(std::cout, density, "\033[1;37;44m");
        std::cout << std::endl;
        density.clear();
        _store.getDensityStats(128, density, 1);
        std::cout << "common map: " << std::endl;
        printGraph(std::cout, density, "\033[1;37;44m");
        std::cout << std::endl;
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

    size_t determineLength(StateID const& s) const {
        return ((typename dtree<HASHSET>::Index*) &s)->getLength();;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 24;
    }

private:
    dtree<HASHSET> _store;
};

} // namespace llmc::storage
