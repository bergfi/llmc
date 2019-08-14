#pragma once

#include <config.h>
#include <assertions.h>

#include <cstring>
#include <unordered_map>

#include "../murmurhash.h"

template<typename StateSlot>
struct FullStateData {

    static FullStateData* create(bool isRoot, size_t length, StateSlot* data) {
        FullStateData* fsd = (FullStateData*)malloc(sizeof(FullStateData<StateSlot>) + length * sizeof(StateSlot));
        fsd->_length = length | (((size_t)isRoot) << 63);
        memcpy(fsd->_data, data, length * sizeof(StateSlot));
        return fsd;
    }

    static FullStateData* create(bool isRoot, size_t length) {
        FullStateData* fsd = (FullStateData*)malloc(sizeof(FullStateData<StateSlot>) + length * sizeof(StateSlot));
        fsd->_length = length | (((size_t)isRoot) << 63);
        return fsd;
    }

    static FullStateData* create(FullStateData* fsd, bool isRoot, size_t length) {
        fsd->_length = length | (((size_t)isRoot) << 63);
        return fsd;
    }

    static void destroy(FullStateData* d) {
        free(d);
    }

    size_t getLength() const {
        return _length & 0x7FFFFFFFFFFFFFFFULL;
    }

    size_t getLengthInBytes() const {
        return getLength() * sizeof(StateSlot);
    }

    const StateSlot* getData() const {
        return _data;
    }

    StateSlot* getData() {
        return _data;
    }

    bool isRoot() const {
        return _length >> 63;
    }

    bool equals(FullStateData const& other) const {
        if(_length != other._length) return false;
        auto r = !memcmp((void*)_data, (void*)other._data, getLength() * sizeof(StateSlot));
        //LLMC_DEBUG_LOG() << "Compared " << *this << " to " << other << ": " << r << std::endl;
        return r;
    }

    size_t hash() const {
        return MurmurHash64((void*)this, sizeof(FullStateData<StateSlot>) + getLength(), 0);
    }

    friend std::ostream& operator<<(std::ostream& os, FullStateData<StateSlot> const& fsd) {
        os << "[";
        os << fsd.getLength();
        os << ",";
        os << fsd.hash();
        os << ",";
        const StateSlot* s = fsd.getData();
        const StateSlot* end = s + fsd.getLength();
        for(; s < end; s++) {
            os << s;
        }
        os << "]" << std::endl;
        return os;
    }

private:
    size_t _length; // in nr of StateSlots
    StateSlot _data[0];
};

class StorageInterface {
public:

    using StateSlot = uint32_t;
    //using StateID = uint64_t;
    using StateTypeID = uint16_t;
    using FullState = FullStateData<StateSlot>;

    struct StateID {
    public:
        constexpr StateID(): _data(0) {}

        constexpr StateID(uint64_t const& d): _data(d) {}
        bool exists() const {
            return _data != 0;
        }

        constexpr uint64_t getData() const {
            return _data;
        }

        static constexpr StateID NotFound() {
            return StateID(0);
        }

        constexpr bool operator==(StateID const& other) const {
            return _data == other._data;
        }

        constexpr bool operator!=(StateID const& other) const {
            return _data != other._data;
        }

    protected:
        uint64_t _data;

    };

    class InsertedState {
    public:

        InsertedState(): _stateID(0), _inserted(0) {}

        InsertedState(StateID stateId, bool inserted): _stateID(stateId), _inserted(inserted) {}

        StateID getState() const {
            return _stateID;
        }

        bool isInserted() const {
            return _inserted;
        }
    private:
        StateID _stateID;
        uint64_t _inserted;
    };

    struct Delta {

        static Delta* create(size_t offset, StateSlot* data, size_t len) {
            Delta* d = (Delta*)malloc(sizeof(Delta) + len * sizeof(StateSlot));
            d->_offset = offset;
            d->_length = len;
            memcpy(d->_data, data, len * sizeof(StateSlot));
            return d;
        }

        static void destroy(Delta* d) {
            free(d);
        }

        size_t getOffset() const {
            return _offset;
        }

        size_t getLength() const {
            return _length;
        }

        size_t getOffsetInBytes() const {
            return getOffset() * sizeof(StateSlot);
        }

        size_t getLengthInBytes() const {
            return getLength() * sizeof(StateSlot);
        }

        const StateSlot* getData() const {
            return _data;
        }

    private:
        size_t _offset;
        size_t _length;
        StateSlot _data[];
    };

    struct MultiDelta {
        size_t length;
    };

    StateID find(FullState* state);
    StateID find(StateSlot* state, size_t length, bool isRoot);
//    InsertedState insertOverwrite(FullState* state);
//    InsertedState insertOverwrite(StateSlot* state, size_t length, bool isRoot);
    InsertedState insert(FullState* state);
    InsertedState insert(StateSlot* state, size_t length, bool isRoot);
    InsertedState insert(StateID const& stateID, Delta const& delta);
    FullState* get(StateID id);
    void printStats();
};

namespace std {

    template <typename StateSlot>
    struct hash<FullStateData<StateSlot>*>
    {
        std::size_t operator()(FullStateData<StateSlot>* const& fsd) const {
            return fsd->hash();
        }
    };

    template <typename StateSlot>
    struct equal_to<FullStateData<StateSlot>*> {
        constexpr bool operator()( FullStateData<StateSlot>* const& lhs, FullStateData<StateSlot>* const& rhs ) const {
            return lhs->equals(*rhs);
        }
    };

    template<>
    struct hash<StorageInterface::StateID>
    {
        std::size_t operator()(StorageInterface::StateID const& s) const {
            return std::hash<uint64_t>()(s.getData());
        }
    };

    template<>
    struct equal_to<StorageInterface::StateID> {
        constexpr bool operator()( StorageInterface::StateID const& lhs, StorageInterface::StateID const& rhs ) const {
            return lhs.getData() == rhs.getData();
        }
    };

//template <typename StateSlot>
//ostream& operator<<(ostream& os, FullStateData<StateSlot> const& fsd) {
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

class CrappyStorage: public StorageInterface {
public:

    CrappyStorage(): _nextID(1), _store(), _storeID() {

    }

    ~CrappyStorage() {
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

private:
    size_t _nextID;
    std::unordered_map<FullState*, StateID> _store;
    std::unordered_map<StateID, FullState*> _storeID;
};
