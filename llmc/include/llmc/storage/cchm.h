#pragma once

#include "interface.h"
#include "../../../../../httest/chaintableUBVKmem.h"
#include <libfrugi/Settings.h>

using namespace libfrugi;

namespace hashtables {

template<>
struct key_accessor<llmc::storage::StorageInterface::FullState> {

    __attribute__((always_inline))
    static size_t size(llmc::storage::StorageInterface::FullState const& key) {
//        printf("length %u\n", key.getLengthInBytes());
        return key.getLengthInBytes() + sizeof(llmc::storage::StorageInterface::FullState);
    }
};

}

namespace llmc::storage {

class cchm: public StorageInterface {
public:

    cchm(): _store(24), _rootMap(nullptr), _hashmapScale(24), _hashmapRootScale(24) {

    }

    ~cchm() {
        size_t buckets = 1ULL << _hashmapRootScale;
        if(_rootMap) MMapper::munmap(_rootMap, buckets * sizeof(*_rootMap));
        _rootMap = nullptr;
    }

    void init() {
        _store.getSlabManager().setMinimumSlabSize(1ULL << _hashmapScale);
        _store.setBucketScale(_hashmapScale);
        size_t buckets = 1ULL << _hashmapRootScale;
        _hashmapRootMask = buckets - 1;
        _rootMap = (decltype(_rootMap))MMapper::mmapForMap(buckets * sizeof(*_rootMap));
        assert(_rootMap);
        _store.init();
    }

    void thread_init() {
        _store.thread_init();
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateID = StorageInterface::StateID;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

    StateID find(const FullState* state) {
        chaintablegenericUBVKmem::HashTable<FullState>::HTE* result;
        if(_store.get(*state, result)) {
            return findInRootMap(result) | (state->getLength() << 40);
        }
        return StateID::NotFound();
    }

    StateID find(const StateSlot* state, size_t length, bool isRoot) {
        auto fsd = FullStateData<StateSlot>::createExternal(isRoot, length, state);
        auto r = find(fsd);
        return r;
    }

    uintptr_t findInRootMap(chaintablegenericUBVKmem::HashTable<FullState>::HTE* id) {
        size_t e = ((uintptr_t)id) & _hashmapRootMask;
        while(true) {
            chaintablegenericUBVKmem::HashTable<FullState>::HTE* loaded = _rootMap[e].load(std::memory_order_relaxed);
            if(loaded == id) {
                break;
            }
            if(!loaded) {
                std::atomic_thread_fence(std::memory_order_acquire);
                loaded = _rootMap[e].load(std::memory_order_relaxed);
                if(loaded == id) {
                    break;
                }
            }
            if(!loaded) {
                std::this_thread::yield();
            } else {
                e++;
            }
        }
        return e;
    }

    //    InsertedState insertOverwrite(FullState* state) {
//        auto id = _nextID;
//        auto r = _store.insert_or_assign(state, id);
//        return {r.first->second, r.second};
//    }
//    InsertedState insertOverwrite(StateSlot* state, size_t length, bool isRoot) {
//        return insert(FullStateData<StateSlot>::create(isRoot, length, state));
//    }

    InsertedState insert(const FullState* state) {
        chaintablegenericUBVKmem::HashTable<FullState>::HTE* id = nullptr;
        bool inserted = _store.insert(*state, id);
        size_t e = ((uintptr_t)id) & _hashmapRootMask;
        if(inserted) {
            chaintablegenericUBVKmem::HashTable<FullState>::HTE* nullValue = nullptr;
            while(!_rootMap[e].compare_exchange_strong(nullValue, id, std::memory_order_release, std::memory_order_relaxed)) {
                if(nullValue == id) {
                    break;
                    inserted = false;
                }
                nullValue = nullptr;
                e++;
            }
        } else {
            e = findInRootMap(id);
        }
        return InsertedState(e | (state->getLength() << 40), inserted);
    }

    InsertedState insert(const StateSlot* state, size_t length, bool isRoot) {
        return insert(FullStateData<StateSlot>::createExternal(isRoot, length, state));
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        return insert(stateID, delta.getOffset(), delta.getLength(), delta.getData(), isRoot);
    }
    InsertedState insert(StateID const& stateID, size_t offset, size_t length, const StateSlot* data, bool isRoot) {
        FullState* old = get(stateID, isRoot);
        LLMC_DEBUG_ASSERT(old);
        LLMC_DEBUG_ASSERT(old->getLength());

//        fprintf(stderr, "old:");
//        for(int i=0; i < old->getLength(); ++i) {
//            fprintf(stderr, " %x", old->getData()[i]);
//        }
//        fprintf(stderr, "\n");

        size_t newLength = std::max(old->getLength(), length + offset);

        StateSlot buffer[newLength];

        memcpy(buffer, old->getData(), old->getLength() * sizeof(StateSlot));
        memcpy(buffer+offset, data, length * sizeof(StateSlot));
        if(offset > old->getLength()) {
            memset(buffer+old->getLength(), 0, (offset - old->getLength()) * sizeof(StateSlot));
        }
//        fprintf(stderr, "new:");
//        for(int i=0; i < newLength; ++i) {
//            fprintf(stderr, " %x", buffer[i]);
//        }
//        fprintf(stderr, "\n");

        return insert(buffer, newLength, isRoot);

    }

    // need to do:
    // - external FullState
    // - FullState needs to be tailored for treedbs






    FullState* get(StateID id, bool isRoot) {
        assert(id.getData());
//        auto hte = (chaintablegenericUBVKmem::HashTable<FullState>::HTE*)id.getData();
        auto hte = _rootMap[id.getData() & 0xFFFFFFFFFFULL].load(std::memory_order_acquire);
//        FullState* state = (FullState*)&hte->_length;
//        auto state = FullState::createExternal(isRoot, hte->_length / sizeof(StateSlot), (StateSlot*)hte->_keyData);
        FullState& state = hte->get();

        if(state.isRoot() == isRoot) {
            return &state;
        } else {
            return nullptr;
        }
    }

    bool get(StateSlot* dest, StateID stateID, bool isRoot) {
        FullState* s = get(stateID, isRoot);
        if(s) {
            memcpy(dest, s->getData(), s->getLengthInBytes());
        }
        return s != nullptr;
    }

    bool getPartial(StateID stateID, size_t offset, StateSlot* dest, size_t length, bool isRoot) {
        FullState* s = get(stateID, isRoot);
        if(s) {
            memcpy(dest, &s->getData()[offset], length * sizeof(StateSlot));
        }
        return s != nullptr;
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

    static bool constexpr needsThreadInit() {
        return true;
    }

    size_t determineLength(StateID const& s) const {
        return s.getData() >> 40;
//        auto hte = (chaintablegenericUBVKmem::HashTable<FullState>::HTE*)s.getData();
//        return hte->_length;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 32;
    }

    void setSettings(Settings& settings) {
        size_t hashmapScale = settings["storage.cchm.hashmap_scale"].asUnsignedValue();
        hashmapScale = hashmapScale ? hashmapScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapScale) {
            _hashmapScale = hashmapScale;
        }

        size_t hashmapRootScale = settings["storage.cchm.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmaproot_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.cchm.hashmap_scale"].asUnsignedValue();
        hashmapRootScale = hashmapRootScale ? hashmapRootScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapRootScale) {
            _hashmapRootScale = hashmapRootScale;
        }
    }

    Statistics getStatistics() {
        Statistics stats;

        stats._bytesReserved = _store.getSlabManager().max_size();
        stats._bytesInUse = _store.getSlabManager().size();
        stats._bytesMaximum = 0;
        stats._elements = 0;

        decltype(_store)::stats s;
        _store.getStats(s);

        stats._elements = s.size;
        stats._bytesInUse += sizeof(uint64_t) * s.usedBuckets;
        stats._bytesReserved += sizeof(uint64_t) * _store.getBuckets();

        stats._bytesReserved += (1ULL << _hashmapRootScale) * sizeof(*_rootMap);
        std::atomic<chaintablegenericUBVKmem::HashTable<FullState>::HTE*>* e = _rootMap;
        std::atomic<chaintablegenericUBVKmem::HashTable<FullState>::HTE*>* eEnd = e + (1ULL << _hashmapRootScale);
        while(e < eEnd) {
            if(e->load(std::memory_order_relaxed)) {
                stats._bytesInUse += sizeof(*_rootMap);
            }
            e++;
        }

        return stats;
    }

    std::string getName() const {
        return "cchm";
    }

private:
    chaintablegenericUBVKmem::HashTable<FullState, std::hash<FullState>, std::equal_to<FullState>> _store;
    std::atomic<chaintablegenericUBVKmem::HashTable<FullState>::HTE*>* _rootMap;
    size_t _hashmapScale;
    size_t _hashmapRootScale;
    size_t _hashmapRootMask;
};

} // namespace llmc::storage
