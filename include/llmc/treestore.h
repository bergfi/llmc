/*
 * LLMC - LLVM IR Model Checker
 * Copyright © 2013-2021 Freark van der Berg
 *
 * This file is part of LLMC.
 *
 * LLMC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * LLMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LLMC.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cassert>
#include <cstring>
#include <cinttypes>
#include <unordered_set>
#include <shared_mutex>
#include <vector>
#include <atomic>

class TypeInstance {
public:

    size_t hash() const {
        //MurmurHashUnaligned2
        size_t hash = std::_Hash_impl::hash(_data, _length);
        return hash;
    }

    bool operator==(TypeInstance const& other) const {
//        printf("  ARE SAME?\n");
//        printf("    addr %p %p\n", this, &other);
//        printf("    hashes %zx %zx\n", this->hash(), other.hash());
//        printf("    length? %zi %zi\n", this->_length, other._length);
//        printf("    memcmp: %i\n", memcmp(this->_data, other._data, other._length));
//        char* data = (char*)_data;
//        char* data_end = data + _length;
//        while(data < data_end) {
//            printf(" %x", *data);
//            data++;
//        }
//        printf("\n");
//        data = (char*)other._data;
//        data_end = data + other._length;
//        while(data < data_end) {
//            printf(" %x", *data);
//            data++;
//        }
//        printf("\n");
        if(this == &other) return true;
        if(this->_length != other._length) return false;
        if(memcmp(this->_data, other._data, _length) != 0) return false;
        return true;
    }

    TypeInstance(TypeInstance const& other) = delete;
    TypeInstance(TypeInstance&& other) {
//        printf("moved-con %p -> %p\n", &other, this);
        if(this != &other) {
            this->_length = other._length;
            this->_id = other._id;
            this->_external = false;
            if(other._external) {
                this->_data = malloc(other._length);
                assert(this->_data);
                memmove(this->_data, other._data, this->_length);
            } else {
                this->_data = other._data;
                other._data = nullptr;
                other._length = 0;
            }
            other._id = 0;
            other._length = 0;
        }
    }
    TypeInstance const& operator=(TypeInstance const& other) = delete;
    void operator=(TypeInstance&& other) {
//        printf("moved-ass %p -> %p\n", &other, this);
        if(this != &other) {
            this->_length = other._length;
            this->_id = other._id;
            this->_external = false;
            if(other._external) {
                this->_data = malloc(other._length);
                assert(this->_data);
                memmove(this->_data, other._data, this->_length);
            } else {
                this->_data = other._data;
                other._data = nullptr;
                other._length = 0;
            }
            other._id = 0;
        }
    }

    TypeInstance(): _data(nullptr), _length(0), _id(0), _external(false) {}
    TypeInstance(void* data, size_t length, bool external = false): _data(data), _length(length), _id(0), _external(external) {}
    TypeInstance(void* data, size_t length, size_t id): _data(data), _length(length), _id(id), _external(false) {}
    ~TypeInstance() {
        if(!_external) free(_data);
    }

    static TypeInstance clone(void* data, size_t length) {
        void* addr = malloc(length);
        memmove(addr, data, length);
        return std::move(TypeInstance(addr, length));
    }

    TypeInstance clone() {
        return std::move(clone(this->_data, this->_length));
    }

    void* data() const { return _data; }

public:
    void* _data;
    size_t _length;
    size_t _id;
    bool _external;
};

namespace std {
template <>
struct hash<TypeInstance>
{
    std::size_t operator()(TypeInstance const& ti) const {
        return ti.hash();
    }
};
}

//template<typename T>
//class unordered_set_mt {
//public:
//    using mutex_type = std::shared_timed_mutex;
//    using read_only_lock  = std::shared_lock<mutex_type>;
//    using updatable_lock = std::unique_lock<mutex_type>;
//    mutex_type mtx;
//    std::unordered_set<T> _items;
//
//    template<typename T2>
//    auto insert(T2&& t) {
//        updatable_lock lock(mtx);
//        return _items.insert(std::forward<T2>(t));
//    }
//
//    template<typename T2>
//    auto find(T2&& t) {
//        read_only_lock lock(mtx);
//        return _items.find(std::forward<T2>(t));
//    }
//
//};

class TypeStore {
public:
    using mutex_type = std::shared_timed_mutex;
    using read_only_lock  = std::shared_lock<mutex_type>;
    using updatable_lock = std::unique_lock<mutex_type>;

    TypeStore(TypeStore const& other) = delete;
    TypeStore(TypeStore&& other) {
        updatable_lock lock(other.mtx);
        this->_typeInstances = std::move(other._typeInstances);
        this->_typeInstancesRef = std::move(other._typeInstancesRef);
    }

    TypeStore(): _typeInstancesRef(), _typeInstances(), mtx() {
        _typeInstancesRef.reserve(256);
        _typeInstancesRef.push_back(nullptr);
        _typeInstances.reserve(256);
    }

    auto insert(TypeInstance&& ti) {
        updatable_lock lock(mtx);
//        printf("TypeStore[%i]\n", typeNo);
        size_t id = _typeInstancesRef.size();
        ti._id = id;
        auto it = _typeInstances.insert(std::forward<TypeInstance>(ti));
        if(!it.second) {
//            printf("  already have it: %zi\n", it.first->_id); fflush(stdout);
//            printf("  hashes %zx %zx\n", it.first->hash(), ti.hash()); fflush(stdout);
//            printf("  length? %zi %zi\n", it.first->_length, ti._length); fflush(stdout);
//            printf("  memcmp: %i\n", memcmp(it.first->_data, ti._data, ti._length)); fflush(stdout);
            return it;
        }
//        printf("  inserted new: %zi length: %i data:", id, it.first->_length); fflush(stdout);
//        char* data = (char*)it.first->_data;
//        char* data_end = data + it.first->_length;
//        while(data < data_end) {
//            printf(" %x", *data);
//            data++;
//        }
//        printf("\n");
        if(id == _typeInstancesRef.max_size()) {
            _typeInstancesRef.reserve(2*id);
        }
        assert(&*it.first);
        _typeInstancesRef.push_back(&*it.first);
        return it;
    }

    TypeInstance const& find(size_t index) {
        read_only_lock lock(mtx);
        assert(0 <= index);
        assert(index < _typeInstancesRef.size());
        TypeInstance const* res = _typeInstancesRef[index];
        assert(res);
        return *res;
    }

    std::unordered_set<TypeInstance>& typeInstances() {
        return _typeInstances;
    }

    std::vector<TypeInstance const*>& typeInstancesRef() {
        return _typeInstancesRef;
    }

    size_t size() {
        read_only_lock lock(mtx);
        return _typeInstancesRef.size() - 1;
    }

public:
    std::vector<TypeInstance const*> _typeInstancesRef;
    std::unordered_set<TypeInstance> _typeInstances;
    mutex_type mtx;
    int typeNo;
};

//template<typename T>
//class tls {
//    tls() {
//        pthread_key_create(&key, nullptr);
//    }
//
//    void operator=(T t) {
//        pthread_setspecific()
//    }
//private:
//    pthread_key_t key;
//};

static constexpr int BUCKET_SIZE_POWER_TWO = 8ULL;
static constexpr int THREAD_SIZE_POWER_TWO = 0ULL;
static constexpr int CHUNKID_POWER_TWO = 24ULL;

static inline size_t getBucketIDFromHeader(size_t header) {
    return header >> (32-BUCKET_SIZE_POWER_TWO);
}
static inline size_t getThreadIDFromHeader(size_t header) {
    return (header >> CHUNKID_POWER_TWO) & ((1ULL<<THREAD_SIZE_POWER_TWO)-1);
}
static inline size_t getChunkIDFromHeader(size_t header) {
    return header & ((1ULL<<CHUNKID_POWER_TWO)-1);
}

class hashtable_bucket_slab {
public:
    hashtable_bucket_slab(hashtable_bucket_slab&& other) {
        assert(0);
    }

    hashtable_bucket_slab(hashtable_bucket_slab* next, size_t max_size = 10000): _insertAt(nullptr), _next(next) {
        _data = new size_t[max_size];
        _insertAt.store(_data, std::memory_order_relaxed);
        _end = _data + max_size;
    }

    ~hashtable_bucket_slab() {
        delete[] _data;
    }

    size_t alignedLenInSizeT(size_t len) {
        return (len+7)/8;
    }

    size_t* insert(size_t id, size_t len, void const*& data) {
        size_t* old = _insertAt.load(std::memory_order_relaxed) + alignedLenInSizeT(len) + 2;
        size_t* next = nullptr;
        do {
            next = old + alignedLenInSizeT(len) + 2;
            if(next > _end) return nullptr;
        } while(!_insertAt.compare_exchange_strong(old, next, std::memory_order_relaxed, std::memory_order_relaxed));

        next[0] = id;
        next[1] = len;
        memmove((void*)(next+2), data, len);

        return next;
    }

    size_t* find(size_t len, void const* data) {
        size_t* e = _data+1;
        while(e < _insertAt) {
            if(*e == len) {
                if(memcmp(e+1, data, len) == 0) {
                    return e-1;
                }
            }
            e += *e + 2;
        }
        return nullptr;
    }

    hashtable_bucket_slab* getNext() {
        return _next.load(std::memory_order_relaxed);
    }

private:
    size_t* _data;
    size_t* _end;
    std::atomic<size_t*> _insertAt;
    std::atomic<hashtable_bucket_slab*> _next;
};

class hashtable_bucket {
    using mutex_type = std::shared_timed_mutex;
    using read_only_lock  = std::shared_lock<mutex_type>;
    using updatable_lock = std::unique_lock<mutex_type>;
public:
    hashtable_bucket(size_t threads = 4): _elements(), _slabs(nullptr), _threadSlabs(threads), _mtx() {
        hashtable_bucket_slab* current = nullptr;
        for(int i = threads; i--;) {
            current = new hashtable_bucket_slab(current);
            _threadSlabs[i] = current;
        }
        _slabs.store(current);
        _elements.reserve(1024);
    }
    hashtable_bucket(hashtable_bucket&& other) {
    }
    bool insert(size_t threadID, size_t*& id, size_t len, void const* data) {
        hashtable_bucket_slab* mySlab = getMySlab(threadID);
        updatable_lock lock(_mtx);
        hashtable_bucket_slab* current = _slabs.load(std::memory_order_relaxed);
        while(current) {
            size_t* e = current->find(len, data);
            if(e) {
                id = e;
                return false;
            }
            current = current->getNext();
        }
        size_t threadIDMasked = threadID << CHUNKID_POWER_TWO;
        size_t newID = _bucketIDMasked ^ threadIDMasked ^ _elements.size();
        while(!(id = mySlab->insert(newID, len, data))) {
            mySlab = linkNewSlab(threadID);
        }
        if(_elements.size() >= _elements.max_size()) {
            _elements.reserve(_elements.max_size()*2);
        }
        _elements.push_back(id);
        return true;
    }

    bool find(size_t threadID, size_t index, size_t& len, void*& data) {
        read_only_lock lock(_mtx);
        assert(getChunkIDFromHeader(index) < _elements.size());
        auto e = _elements[getChunkIDFromHeader(index)];
        assert(e);
        assert(*e == index);
        len = *(e+1);
        data = (void*)(e+2);
        return true;
    }

    hashtable_bucket_slab* getMySlab(size_t threadID) {
        return _threadSlabs[threadID];
    }

    hashtable_bucket_slab* linkNewSlab(size_t threadID) {
        hashtable_bucket_slab* old = _lastSlab.load(std::memory_order_relaxed);
        hashtable_bucket_slab* current = new hashtable_bucket_slab(nullptr);
        // barrier SS
        while(!_slabs.compare_exchange_weak(old, current, std::memory_order_release, std::memory_order_relaxed)) {
            ;
        }
        _threadSlabs[threadID] = current;
        return _threadSlabs[threadID];
    }

    void setBucketID(size_t bucketID) {
        _bucketID = bucketID;
        _bucketIDMasked = bucketID << (32-BUCKET_SIZE_POWER_TWO);
    }

private:
    size_t _bucketID;
    size_t _bucketIDMasked;
    std::vector<size_t*> _elements;
    std::atomic<hashtable_bucket_slab*> _slabs;
    std::atomic<hashtable_bucket_slab*> _lastSlab;
    std::vector<hashtable_bucket_slab*> _threadSlabs;
    mutex_type _mtx;
};

template<int t>
static size_t bucketIDFromHash(size_t);

//template<>
//size_t bucketIDFromHash<16>(size_t hash) {
//    size_t hash32 = hash >> 32;
//    hash32 ^= hash;
//    hash = hash32 >> 16;
//    hash32 ^= hash;
//    return hash32 & 0xFFFF;
//}
//
//template<>
//size_t bucketIDFromHash<20>(size_t hash) {
//    size_t hash2 = hash >> 24;
//    hash2 ^= hash;
//    hash = hash2 >> 20;
//    hash2 ^= hash;
//    return hash2 & 0xFFFFF;
//}

//template<>
//size_t bucketIDFromHash<24>(size_t hash) {
//    size_t hash2 = hash >> 16;
//    hash2 ^= hash;
//    hash = hash2 >> 24;
//    hash2 ^= hash;
//    return hash2 & 0xFFFFFF;
//}

template<>
size_t bucketIDFromHash<8>(size_t hash) {
    size_t hash2 = hash >> 32;
    hash2 ^= hash;
    hash = hash2 >> 16;
    hash2 ^= hash;
    hash2 = hash >> 8;
    hash2 ^= hash;
    return hash2 & 0xFF;
}

class hashtable {
public:
public:

    hashtable() {
        buckets.resize(1<<BUCKET_SIZE_POWER_TWO);
        for(size_t i=buckets.size(); i--;) {
            buckets[i].setBucketID(i);
        }
    }

    bool insert(size_t threadID, size_t*& id, size_t len, void const* data) {
        size_t hash = std::_Hash_impl::hash(data, len);
        size_t bucketID = bucketIDFromHash<BUCKET_SIZE_POWER_TWO>(hash);
        return buckets[bucketID].insert(threadID, id, len, data);
    }

    bool find(size_t threadID, size_t index, size_t& len, void*& data) {
        size_t bucketID = getBucketIDFromHeader(index);
        return buckets[bucketID].find(threadID, index, len, data);
    }

    size_t size() {
        return 0;
    }

private:
    std::vector<hashtable_bucket> buckets;
};

class TreeStore {
public:

    TreeStore(): _typeDB() {
        _typeDB.resize(1);
        _typeDB[0].typeNo = 0;
    }

    auto insert(size_t type, TypeInstance&& ti) {
        if(type >= _typeDB.size()) _typeDB.resize(type+1);
        _typeDB[type].typeNo = type;
        return _typeDB[type].insert(std::forward<TypeInstance>(ti));
    }
    auto insert(size_t threadID, size_t type, size_t len, void* data, bool external = false) {
        return insert(type, TypeInstance(data, len, external));
    }

    TypeInstance const& find(size_t type, size_t index) {
        return _typeDB[type].find(index);
    }

    void printStats() {
        int i=0;
        for(auto& ts: _typeDB) {
            printf("[TS:%2i]: %6zi instances\n", i++, ts.typeInstancesRef().size()-1);
        }
    }

    TypeStore& typeDB(size_t type) {
        return _typeDB[type];
    }

private:
    std::vector<TypeStore> _typeDB;

};

class TreeStore_2 {
public:

    TreeStore_2(size_t types): _typeDB(types) {
    }

    auto insert(size_t threadID, size_t type, size_t*& index, size_t len, void const* data, bool external = false) {
        assert(type < _typeDB.size());
        return _typeDB[type].insert(threadID, index, len, data);
    }

    bool find(size_t threadID, size_t type, size_t index, size_t& len, void*& data) {
        assert(type < _typeDB.size());
        return _typeDB[type].find(threadID, index, len, data);
    }

    void printStats() {
        int i=0;
        for(auto& ts: _typeDB) {
            printf("[TS:%2i]: %6zi instances\n", i++, ts.size());
        }
    }

    hashtable& typeDB(size_t type) {
        assert(type < _typeDB.size());
        return _typeDB[type];
    }

private:
    std::vector<hashtable> _typeDB;

};
