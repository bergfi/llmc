#pragma once

#include <config.h>
#include <assertions.h>

#include <cstring>

#include "../murmurhash.h"

namespace llmc::storage {

template<typename StateSlot>
struct FullStateData {

    static const size_t OFFSET_TYPE = 0;
    static const size_t BITS_TYPE = 24;
    static const size_t MASK_TYPE = ((1ULL << BITS_TYPE) - 1) << OFFSET_TYPE;

    static const size_t MASK_EXTERNALDATA = (1ULL << 31);

    static FullStateData* createExternal(bool isRoot, size_t length, const StateSlot* data) {
        FullStateData* fsd = (FullStateData*)malloc(sizeof(FullStateData));
        fsd->_misc = (((size_t)!isRoot) << OFFSET_TYPE) | MASK_EXTERNALDATA;
        fsd->_length = length;
        fsd->_externalData = data;
        return fsd;
    }

    static FullStateData* create(bool isRoot, size_t length, StateSlot* data) {
        FullStateData* fsd = (FullStateData*)malloc(sizeof(_header) + length * sizeof(StateSlot));
        fsd->_misc = (((size_t)!isRoot) << OFFSET_TYPE);
        fsd->_length = length;
        memcpy(fsd->_data, data, length * sizeof(StateSlot));
        return fsd;
    }

    static FullStateData* create(bool isRoot, size_t length) {
        FullStateData* fsd = (FullStateData*)malloc(sizeof(_header) + length * sizeof(StateSlot));
        fsd->_misc = (((size_t)!isRoot) << OFFSET_TYPE);
        fsd->_length = length;
        return fsd;
    }

    static FullStateData* create(FullStateData* fsd, bool isRoot, size_t length) {
        fsd->_misc = (((size_t)!isRoot) << OFFSET_TYPE);
        fsd->_length = length;
        return fsd;
    }

    static void destroy(FullStateData* d) {
        free(d);
    }

    size_t getLength() const {
        return _length;
    }

    size_t getLengthInBytes() const {
        return getLength() * sizeof(StateSlot);
    }

    const StateSlot* getData() const {
        return _misc & MASK_EXTERNALDATA ? _externalData : _data;
    }

    const char* getCharData() const {
        return (char*)getData();
    }

    StateSlot* getDataToModify() {
        assert(!(_misc & MASK_EXTERNALDATA));
        return _data;
    }

    bool isRoot() const {
        return !(_misc & MASK_TYPE);
    }

    bool equals(FullStateData const& other) const {
        if(_header != other._header) return false;

        auto thisData = _misc & MASK_EXTERNALDATA ? (void*)_externalData : (void*)_data;
        auto otherData = other._misc & MASK_EXTERNALDATA ? (void*)other._externalData : (void*)other._data;

        auto r = !memcmp(thisData, otherData, getLength() * sizeof(StateSlot));
        //LLMC_DEBUG_LOG() << "Compared " << *this << " to " << other << ": " << r << std::endl;
        return r;
    }

    size_t hash() const {
        return _header ^ MurmurHash64(_misc & MASK_EXTERNALDATA ? _externalData : _data, getLength(), 0);
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
    union {
        struct {
            uint32_t _length; // in nr of StateSlots
            uint32_t _misc;
        };
        size_t _header;
    };
    union {
        StateSlot _data[0];
        const StateSlot* _externalData;
    };
};

//static_assert(sizeof(FullStateData<unsigned int>) == sizeof(size_t));

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

        static Delta& create(Delta* buffer, size_t offset, StateSlot* data, size_t len) {
            buffer->_offset = offset;
            buffer->_length = len;
            memcpy(buffer->_data, data, len * sizeof(StateSlot));
            return *buffer;
        }

        static Delta* create(size_t offset, StateSlot* data, size_t len) {
            Delta* d = (Delta*)malloc(sizeof(Delta) + len * sizeof(StateSlot));
            create(d, offset, data, len);
            return d;
        }

        static Delta& create(Delta* buffer, size_t offset, size_t len) {
            return Delta::create(buffer, offset, buffer->_data, len);
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

//    StateID find(FullState* state);
//    StateID find(StateSlot* state, size_t length, bool isRoot);
////    InsertedState insertOverwrite(FullState* state);
////    InsertedState insertOverwrite(StateSlot* state, size_t length, bool isRoot);
//    InsertedState insert(FullState* state);
//    InsertedState insert(StateSlot* state, size_t length, bool isRoot);
//    InsertedState insert(StateID const& stateID, Delta const& delta);
//    FullState* get(StateID id);
//    void printStats();
};

} // namespace llmc::storage