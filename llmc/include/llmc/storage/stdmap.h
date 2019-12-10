#pragma once

#include <unordered_map>

#include "interface.h"

namespace std {

template <typename StateSlot>
struct hash<llmc::storage::FullStateData<StateSlot>*>
{
std::size_t operator()(llmc::storage::FullStateData<StateSlot>* const& fsd) const {
    return fsd->hash();
}
};

template <typename StateSlot>
struct equal_to<llmc::storage::FullStateData<StateSlot>*> {
constexpr bool operator()( llmc::storage::FullStateData<StateSlot>* const& lhs, llmc::storage::FullStateData<StateSlot>* const& rhs ) const {
    return lhs->equals(*rhs);
}
};

template<>
struct hash<llmc::storage::StorageInterface::StateID>
{
    std::size_t operator()(llmc::storage::StorageInterface::StateID const& s) const {
        return std::hash<uint64_t>()(s.getData());
    }
};

template<>
struct equal_to<llmc::storage::StorageInterface::StateID> {
    constexpr bool operator()( llmc::storage::StorageInterface::StateID const& lhs, llmc::storage::StorageInterface::StateID const& rhs ) const {
        return lhs.getData() == rhs.getData();
    }
};

//template <typename StateSlot>
//ostream& operator<<(ostream& os, llmc::storage::FullStateData<StateSlot> const& fsd) {
//    os << "[";
//    os << fsd.getLength();
//    os << ",";
//    os << fsd.hash();
//    os << ",";
//    const StateSlot* s = fsd.getData();
//    const StateSlot* end = s + fsd.getLength();
//    for(; s < end; s++) {
//        os << s;
//    }
//    os << "]" << std::endl;
//    return os;
//}

}

namespace llmc::storage {

class StdMap: public StorageInterface {
public:

    StdMap(): _nextID(1), _store(), _storeID() {

    }

    ~StdMap() {
        for(auto& s: _store) {
            free(s.first);
        }
    }

    void init() {
        _store.reserve(1U << 24);
        _storeID.reserve(1U << 24);
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateID = StorageInterface::StateID;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

    StateID find(FullState* state) {
        auto it = _store.find(state);
        if(it == _store.end()) {
            return StateID::NotFound();
        }
        return it->second;
    }
    StateID find(StateSlot* state, size_t length, bool isRoot) {
        auto fsd = FullStateData<StateSlot>::create(isRoot, length, state);
        auto r = find(fsd);
        FullStateData<StateSlot>::destroy(fsd);
        return r;
    }
//    InsertedState insertOverwrite(FullState* state) {
//        auto id = _nextID;
//        auto r = _store.insert_or_assign(state, id);
//        return {r.first->second, r.second};
//    }
//    InsertedState insertOverwrite(StateSlot* state, size_t length, bool isRoot) {
//        return insert(FullStateData<StateSlot>::create(isRoot, length, state));
//    }
    InsertedState insert(FullState* state) {
        auto id = _nextID;
        auto r = _store.insert({state, id});
        if(r.second) {
            ++_nextID;
            _storeID[id] = state;
            //LLMC_DEBUG_LOG() << "Inserted state " << state << " -> " << id << std::endl;
        } else {
            //LLMC_DEBUG_LOG() << "Inserted state " << state << " -> " << r.first->second << "(already inserted)" << std::endl;
        }
        return InsertedState(r.first->second, r.second);
    }
    InsertedState insert(StateSlot* state, size_t length, bool isRoot) {
        return insert(FullStateData<StateSlot>::create(isRoot, length, state));
    }
    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        FullState* old = get(stateID, isRoot);
        LLMC_DEBUG_ASSERT(old);
        LLMC_DEBUG_ASSERT(old->getLength());

        size_t newLength = std::max(old->getLength(), delta.getLength() + delta.getOffset());

        StateSlot buffer[newLength];

        memcpy(buffer, old->getData(), old->getLength() * sizeof(StateSlot));
        memcpy(buffer+delta.getOffset(), delta.getData(), delta.getLength() * sizeof(StateSlot));

        return insert(buffer, newLength, isRoot);

    }

    FullState* get(StateID id, bool isRoot) {
        auto it = _storeID.find(id);
        if(it == _storeID.end()) {
            abort();
            return nullptr;
        }
        return it->second;
    }

    void printStats() {
    }

    static bool constexpr accessToStates() {
        return true;
    }

    static bool constexpr stateHasFixedLength() {
        return false;
    }

    static bool constexpr stateHasLengthInfo() {
        return false;
    }

    size_t determineLength(StateID const& s) const {
        return 0;
    }

    static bool constexpr needsThreadInit() {
        return false;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 32;
    }

private:
    size_t _nextID;
    ::std::unordered_map<FullState*, StateID> _store;
    ::std::unordered_map<StateID, FullState*> _storeID;
};

} // namespace llmc::storage