#pragma once

#include <cstring>
#include <shared_mutex>
#include <unordered_map>

#include "interface.h"
#include <libfrugi/Settings.h>
#include <allocator.h>

using namespace libfrugi;

namespace llmc::storage {

template<typename T>
class countingallocator: std::allocator<T> {
public:

    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    template<class U> struct rebind {
        typedef countingallocator<U> other;
    };

    countingallocator(): bytesUsed(0) {
    }

    countingallocator(const countingallocator<T>& other) throw(): bytesUsed(0) {
    }

    template<typename U> countingallocator(const countingallocator<U>& other) throw() {
        bytesUsed = other.size();
    }

    template<typename U> countingallocator(countingallocator<U>&& other) throw() {
        bytesUsed = other.size();
    }

    void operator=(const countingallocator<T>& other) {
        bytesUsed = other.size();
    }

    void operator=(countingallocator<T>&& other) {
        bytesUsed = other.size();
    }

    void deallocate(pointer p, size_type n) {
        bytesUsed -= n * sizeof(T);
        return std::allocator<T>::deallocate(p, n);
    }

    pointer allocate(size_type n, const void* hint =0 ) {
        bytesUsed += n * sizeof(T);
        return std::allocator<T>::allocate(n, hint);
    }

    size_t size() const {
        return bytesUsed;
    }

private:
    size_t bytesUsed;
};

class StdMap: public StorageInterface {
public:
    using mutex_type = std::shared_timed_mutex;
    using read_only_lock  = std::shared_lock<mutex_type>;
    using updatable_lock = std::unique_lock<mutex_type>;
public:

    StdMap(): _nextID(1),_hashmapScale(24), _store(), _storeID() {

    }

    ~StdMap() {
        for(auto& s: _store) {
            free(s.first);
        }
    }

    void init() {
        _store.reserve(1U << _hashmapScale);
        _storeID.reserve(1U << _hashmapScale);
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateID = StorageInterface::StateID;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

    StateID find(FullState* state) {
        read_only_lock lock(mtx);
        auto it = _store.find(state);
//        printf("finding state");
//        auto b = state->getData();
//        auto e = state->getData() + state->getLength();
//        while(b < e) {
//            printf(" %x", *b);
//            b++;
//        }
//        printf(" -> %u\n", it != _store.end());
        if(it == _store.end()) {
            return StateID::NotFound();
        }
        return it->second;
    }
    StateID find(StateSlot* state, size_t length, bool isRoot) {
        auto fsd = FullStateData<StateSlot>::createExternal(isRoot, length, state);
        auto r = find(fsd);
        fsd->destroy();
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
        updatable_lock lock(mtx);
        auto id = _nextID | (state->getLength() << 40);
        auto r = _store.insert({state, id});
        if(r.second) {
            ++_nextID;
            _storeID[id] = state;
//            printf("inserted state");
//            auto b = state->getData();
//            auto e = state->getData() + state->getLength();
//            while(b < e) {
//                printf(" %x", *b);
//                b++;
//            }
//            printf("\n");
            //LLMC_DEBUG_LOG() << "Inserted state " << state << " -> " << id << std::endl;
        } else {
            //LLMC_DEBUG_LOG() << "Inserted state " << state << " -> " << r.first->second << "(already inserted)" << std::endl;
        }
        return InsertedState(r.first->second, r.second);
    }
    InsertedState insert(const StateSlot* state, size_t length, bool isRoot) {
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
        if(delta.getOffset() > old->getLength()) {
            memset(buffer+old->getLength(), 0, (delta.getOffset() - old->getLength()) * sizeof(StateSlot));
        }

        return insert(buffer, newLength, isRoot);

    }
    InsertedState insert(StateID const& stateID, size_t offset, size_t length, const StateSlot* data, bool isRoot) {
        FullState* old = get(stateID, isRoot);
                LLMC_DEBUG_ASSERT(old);
                LLMC_DEBUG_ASSERT(old->getLength());

        size_t newLength = std::max(old->getLength(), length + offset);

        StateSlot buffer[newLength];

        memcpy(buffer, old->getData(), old->getLength() * sizeof(StateSlot));
        memcpy(buffer+offset, data, length * sizeof(StateSlot));
        if(offset > old->getLength()) {
            memset(buffer+length, 0, (offset - old->getLength()) * sizeof(StateSlot));
        }

        return insert(buffer, newLength, isRoot);

    }

    FullState* get(StateID id, bool isRoot) {
        read_only_lock lock(mtx);
        auto it = _storeID.find(id);
        if(it == _storeID.end()) {
            printf("Cannot find %zx\n", id.getData()); fflush(stdout);
            abort();
            return nullptr;
        }
        return it->second;
    }

    bool get(StateSlot* dest, StateID stateID, bool isRoot) {
        FullState* s = get(stateID, isRoot);
        if(s) {
            memcpy(dest, s->getData(), s->getLengthInBytes());
        }
        return s != nullptr;
    }

    bool getPartial(StateID id, size_t offset, StateSlot* data, size_t length, bool isRoot) {
        FullState* s = get(id, isRoot);
        if(s) {
            memcpy(data, &s->getData()[offset], length * sizeof(StateSlot));
        }
        return s != nullptr;
//        read_only_lock lock(mtx);
//        auto it = _storeID.find(id);
//        if(it == _storeID.end()) {
//            return false;
//        }
//
//        memcpy(data, &it->second->getData()[offset], length * sizeof(StateSlot));
//
//        return true;
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
        return true;
    }

    size_t determineLength(StateID const& s) const {
        return s.getData() >> 40;
    }

    static bool constexpr needsThreadInit() {
        return false;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 32;
    }

    void setSettings(Settings& settings) {
        size_t hashmapScale = settings["storage.stdmap.hashmap_scale"].asUnsignedValue();
        hashmapScale = hashmapScale ? hashmapScale : settings["storage.hashmap_scale"].asUnsignedValue();
        if(hashmapScale) {
            _hashmapScale = hashmapScale;
        }
    }

    Statistics getStatistics() {
        Statistics stats;
        stats._bytesReserved = 0;
        stats._bytesInUse = 0;
        stats._bytesMaximum = 0;
        stats._elements = _store.size();

        for(auto& pair: _store) {
            stats._bytesInUse += pair.first->getLengthInBytes();
            stats._bytesReserved += pair.first->getLengthInBytes();
        }

        // Buckets
        stats._bytesInUse += stats._elements * sizeof(uint64_t*);
        stats._bytesReserved += _store.bucket_count() * sizeof(uint64_t*);

        // Entry pairs
        stats._bytesInUse += _store.get_allocator().size();
        stats._bytesReserved += _store.get_allocator().size();

        // rough estimate...
        return stats;

    }

    std::string getName() const {
        return "stdmap";
    }

private:
    mutex_type mtx;
    size_t _nextID;
    size_t _hashmapScale;
    ::std::unordered_map<FullState*, StateID, std::hash<FullState*>, std::equal_to<FullState*>, countingallocator<std::pair<const FullState*, StateID>>> _store;
    ::std::unordered_map<StateID, FullState*, std::hash<StateID>, std::equal_to<StateID>, countingallocator<std::pair<StateID, const FullState*>>> _storeID;
};

} // namespace llmc::storage