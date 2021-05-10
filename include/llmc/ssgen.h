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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>

#include <deque>
#include <dlfcn.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

extern "C" {
#include <ltsmin/pins.h>
}

#include <libfrugi/System.h>
#include <llmc/treestore.h>
#include <thread>
#include <atomic>


uint32_t SuperFastHash (const char * data, int len);

class ssgen;


class model {
public:
    static model* getPINS(std::string const& filename);
    virtual int getNextAll(TypeInstance const& s, ssgen* ss) = 0;
    virtual int getNextAll(size_t s, void const* data, ssgen* ss) = 0;
    virtual void getInitial(TypeInstance& s, ssgen* ss) = 0;
};

class model_pins_so: public model {
public:
    typedef void(*model_init_t)(void* model);
    //typedef int(*next_method_grey_t)(void* self,int, void*src,void* cb,void*user_context);
    //typedef int(*next_method_black_t)(void* self,void*src,void* cb,void*user_context);

    model_pins_so(void* handle): _init(), _handle(handle) {
        _pins_model_init = reinterpret_cast<decltype(_pins_model_init)>(dlsym(_handle, "pins_model_init"));
        //_pins_getnextall = reinterpret_cast<decltype(_pins_getnextall)>(dlsym(_handle, "pins_getnextall"));
    }

    void setGetNext(next_method_grey_t f) {
        _pins_getnext = f;
    }

    static model_pins_so* get(std::string const& filename) {
        auto handle = dlopen(filename.c_str(), RTLD_LAZY);
        if(!handle) {
            printf("Failed to open %s: %s\n", filename.c_str(), dlerror());
            return nullptr;
        }
        return new model_pins_so(handle);
    }

    int getNextAll(TypeInstance const& s, ssgen* ss);
    int getNextAll(size_t len, void const* data, ssgen* ss);

    void setInitialState(int* s) {
        //int* data = (int*)malloc(sizeof(int) + _length);
        //assert(data);
        //*data = _length;
        //memmove(data+1, s, _length);
        assert(s);
        _init = std::move(TypeInstance(s, _length, true));
        printf("SET INITIAL %zx\n", _init.hash()); fflush(stdout);
    }

    void getInitial(TypeInstance& s, ssgen* ss) {
        _pins_model_init((void*)ss);
        assert(_init.data());
        s = _init.clone();
    }

    void setLength(int length) {
        _length = length;
    }

    int getLength() const { return _length; }

    void setTransitionGroups(int tgroups) {
        _tgroups = tgroups;
    }

private:
    int _length;
    int _tgroups;
    TypeInstance _init;
    model_init_t _pins_model_init;
    next_method_grey_t _pins_getnext;
    next_method_black_t _pins_getnextall;
    void* _handle;
};

//namespace std {
//template <>
//struct hash<State>
//{
//    std::size_t operator()(State const& state) const {
//        return state.hash();
//    }
//};
//}

extern int doPrint;
#define printf2(...) do { if(doPrint) { fprintf(stdout, __VA_ARGS__); } } while(false);

class ssgen {
public:
    ssgen(model* m): _m(m) {

//        std::vector<void*> maps;
//        intptr_t addrStart = 0xffff880000000000ULL;
//        void* addr = (void*)addrStart;
//        size_t numberOfPages = 1000000;
//        addr = mmap(nullptr, 4096ULL * numberOfPages, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//        printf("INITIAL Mapping %16zx\n", (intptr_t)addr);
//        for(size_t i=0; i < numberOfPages; ++i) {
//            void* newAddr = mmap(addr, 4096, PROT_READ|PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//            if(newAddr ==  (void*)addr) {
////                printf("ok\n");
//            } else {
//                printf("Mapping %8i %16zx:", i, (intptr_t)addr);
//                printf("FAIL: %16zx\n", (intptr_t)newAddr);
//                static int fails = 0;
//                if(i > 0 && fails++ > 10) {
//                    exit(-1);
//                }
//            }
//            addr = (void*)(((intptr_t)newAddr) + 4096);
//        }
//            exit(0);

    }

    model* getModel() const { return _m; }

    TransitionCB getCB_ReportTransition() {
        return _reportTransition_cb;
    }

    virtual int pins_chunk_put(void* ctx, int type, chunk c) = 0;
    virtual chunk pins_chunk_get(void* ctx, int type, int idx) = 0;
    virtual int pins_chunk_cam(void* ctx, int type, int idx, int offset, char* data, int len) = 0;

protected:
    void setCB_ReportTransition(TransitionCB cb) {
        _reportTransition_cb = cb;
    }
protected:
    model* _m;
    TransitionCB _reportTransition_cb;
};

class ssgen_st: public ssgen {
public:
    ssgen_st(model* m): ssgen(m), _transitions(0) {
        setCB_ReportTransition(reportTransitionCB);
    }

    void go() {
        TypeInstance init;
        _m->getInitial(init, this);
        auto it = _treeStore.insert(0, init.clone());
        stateQueueNew.emplace_back(&*it.first);
        System::Timer timer;
        int level = 0;
        do {
            int level_states = 0;
            //printf("Level %x...\n", level); fflush(stdout);
            stateQueueNew.swap(stateQueue);
            do {
                TypeInstance const* s = stateQueue.front();
                printf2("Go %zx\n", s->hash()); fflush(stdout);
                stateQueue.pop_front();
                _m->getNextAll(*s, this);
                level_states++;
            } while(!stateQueue.empty());
            //printf("  Lvl %x: %x\n", level, level_states); fflush(stdout);
            level++;
        } while(!stateQueueNew.empty());
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %i transitions\n", _treeStore.typeDB(0).size(), _transitions);
        printf("States/s: %lf\n", ((double)_treeStore.typeDB(0).size())/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Tree Store Stats:\n");
        _treeStore.printStats();
    }

    void reportTransition(void* tinfo, void* state, size_t length, void* changes) {
        _transitions++;
        auto it = _treeStore.insert(0, TypeInstance(state, length, true));
        printf2("Report %zx\n", it.first->hash()); fflush(stdout);
        if(it.second) {
            printf2("  NEW!\n"); fflush(stdout);
            stateQueueNew.emplace_back(&*it.first);
        }
    }

    TreeStore& treeStore() { return _treeStore; }

    model* getModel() const { return _m; }

    void static reportTransitionCB(void* s, transition_info* tinfo, int* state, int* changes) {
        ssgen_st* ss = static_cast<ssgen_st*>(s);
        model_pins_so* model = reinterpret_cast<model_pins_so*>(ss->getModel());
        ss->reportTransition(tinfo, state, model->getLength(), changes);
    }

    virtual int pins_chunk_put(void* ctx, int type, chunk c) {
        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
        printf2("pins_chunk_put\n");
        auto it = ss->treeStore().insert(type+1, TypeInstance(c.data, c.len, true));
        printf2("pins_chunk_put %p %i %zi %i %p\n", ctx, type, it.first->_id, c.len, c.data);
        return it.first->_id;
    }

    virtual chunk pins_chunk_get(void* ctx, int type, int idx) {
        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
        printf2("pins_chunk_get\n");
        auto const& ti = ss->treeStore().find(type+1, idx);
        printf2("pins_chunk_get %p %i %i %zi %p\n", ctx, type, idx, ti._length, ti._data);
        return chunk{(unsigned int)ti._length, (char*)ti._data};
    }

    virtual int pins_chunk_cam(void* ctx, int type, int idx, int offset, char* data, int len) {
        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
        auto const& ti = ss->treeStore().find(type+1, idx);
        int newLength = (size_t)(offset + len) > ti._length ? offset + len : ti._length;
        char* copy = (char*)alloca(newLength);
        memmove(copy, ti._data, ti._length);
        memmove(copy+offset, data, len);
        auto it = ss->treeStore().insert(type+1, TypeInstance(copy, newLength, true));
        return it.first->_id;
    }

//    virtual int pins_chunk_cam(void* ctx, int type, int idx, int offset, char* data, int len) {
//        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
//        auto it = ss->treeStore().update(type+1, idx, offset, data, len);
//        return it.first->_id;
//    }


protected:
    std::deque<TypeInstance const*> stateQueue;
    std::deque<TypeInstance const*> stateQueueNew;
    int _transitions;
    TreeStore _treeStore;
};

class ssgen_st_2: public ssgen {
public:
    ssgen_st_2(model* m): ssgen(m), _transitions(0), _treeStore(20) {
        setCB_ReportTransition(reportTransitionCB);
    }

    void go() {
        TypeInstance init;
        _m->getInitial(init, this);
        size_t* s;
        _treeStore.insert(0, 0, s, init._length, init._data);
        stateQueueNew.emplace_back(s);
        System::Timer timer;
        int level = 0;
        do {
            int level_states = 0;
            //printf("Level %x...\n", level); fflush(stdout);
            stateQueueNew.swap(stateQueue);
            do {
                size_t* s = stateQueue.front();
                printf2("Go %p\n", s); fflush(stdout);
                stateQueue.pop_front();
                _m->getNextAll(s[1], (void*)(s+2), this);
                level_states++;
            } while(!stateQueue.empty());
            //printf("  Lvl %x: %x\n", level, level_states); fflush(stdout);
            level++;
        } while(!stateQueueNew.empty());
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %i transitions\n", _treeStore.typeDB(0).size(), _transitions);
        printf("States/s: %lf\n", ((double)_treeStore.typeDB(0).size())/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Tree Store Stats:\n");
        _treeStore.printStats();
    }

    void reportTransition(void* tinfo, void* state, size_t length, void* changes) {
        _transitions++;
        size_t* s;
        auto inserted = _treeStore.insert(0, 0, s, length, state, true);
        printf2("Report %p\n", s); fflush(stdout);
        if(inserted) {
            printf2("  NEW!\n"); fflush(stdout);
            stateQueueNew.emplace_back(s);
        }
    }

    TreeStore_2& treeStore() { return _treeStore; }

    model* getModel() const { return _m; }

    void static reportTransitionCB(void* s, transition_info* tinfo, int* state, int* changes) {
        ssgen_st_2* ss = static_cast<ssgen_st_2*>(s);
        model_pins_so* model = reinterpret_cast<model_pins_so*>(ss->getModel());
        ss->reportTransition(tinfo, state, model->getLength(), changes);
    }

    virtual int pins_chunk_put(void* ctx, int type, chunk c) {
        ssgen_st_2* ss = static_cast<ssgen_st_2*>(ctx);
        printf2("pins_chunk_put\n");
        size_t* s;
        ss->treeStore().insert(0, type+1, s, c.len, c.data, true);
        printf("pins_chunk_put %p %i %zx %i %p\n", ctx, type, *s, c.len, c.data);
        return *s;
    }

    virtual chunk pins_chunk_get(void* ctx, int type, int idx) {
        ssgen_st_2* ss = static_cast<ssgen_st_2*>(ctx);
        printf2("pins_chunk_get\n");
        size_t len;
        void* data;
        ss->treeStore().find(0, type+1, (size_t)idx, len, data);
        printf("pins_chunk_get %p %i %i %zi %p\n", ctx, type, idx, len, data);
        return chunk{(unsigned int)len, (char*)data};
    }

    virtual int pins_chunk_cam(void* ctx, int type, int idx, int offset, char* data, int len) {
        ssgen_st_2* ss = static_cast<ssgen_st_2*>(ctx);
        size_t lenBase;
        void* dataBase;
        ss->treeStore().find(0, type+1, (size_t)idx, lenBase, dataBase);
        int newLength = (size_t)(offset + len) > lenBase ? offset + len : lenBase;
        char* copy = (char*)alloca(newLength);
        memmove(copy, dataBase, lenBase);
        memmove(copy+offset, data, len);
        size_t* s;
        ss->treeStore().insert(0, type+1, s, newLength, copy, true);
        return *s;
    }

protected:
    std::deque<size_t*> stateQueue;
    std::deque<size_t*> stateQueueNew;
    int _transitions;
    TreeStore_2 _treeStore;
};

class ssgen_mt: public ssgen {
    using mutex_type = std::shared_timed_mutex;
    using read_only_lock  = std::shared_lock<mutex_type>;
    using updatable_lock = std::unique_lock<mutex_type>;
public:
    ssgen_mt(model* m): ssgen(m), _transitions(0) {
        setCB_ReportTransition(reportTransitionCB);

//        std::vector<void*> maps;
//        intptr_t addrStart = 0xffff880000000000ULL;
//        void* addr = (void*)addrStart;
//        size_t numberOfPages = 1000000;
//        addr = mmap(nullptr, 4096ULL * numberOfPages, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//        printf("INITIAL Mapping %16zx\n", (intptr_t)addr);
//        for(size_t i=0; i < numberOfPages; ++i) {
//            void* newAddr = mmap(addr, 4096, PROT_READ|PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//            if(newAddr ==  (void*)addr) {
////                printf("ok\n");
//            } else {
//                printf("Mapping %8i %16zx:", i, (intptr_t)addr);
//                printf("FAIL: %16zx\n", (intptr_t)newAddr);
//                static int fails = 0;
//                if(i > 0 && fails++ > 10) {
//                    exit(-1);
//                }
//            }
//            addr = (void*)(((intptr_t)newAddr) + 4096);
//        }
//            exit(0);

    }

    void go() {
        TypeInstance init;
        _m->getInitial(init, this);
        auto it = _treeStore.insert(0, init.clone());
        stateQueueNew.emplace_back(&*it.first);
        System::Timer timer;

        // vector container stores threads
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; i++) {
            workers.push_back(std::thread([this]()
            {
                do {
                    TypeInstance const* current = nullptr;
                    current = getNextQueuedState();
                    if(!current) break;
                    do {
                        printf2("Go %zx\n", current->hash()); fflush(stdout);
                        _m->getNextAll(*current, this);
                        current = nullptr;
                    } while(current);
                } while(true);
            }
            ));
        }
        for (int i = 0; i < 4; i++) {
            workers[i].join();
        }
        auto elapsed = timer.getElapsedSeconds();
        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %i transitions\n", _treeStore.typeDB(0).size(), _transitions.load());
        printf("States/s: %lf\n", ((double)_treeStore.typeDB(0).size())/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions)/elapsed);
        printf("Tree Store Stats:\n");
        _treeStore.printStats();
    }

    void reportTransition(void* tinfo, void* state, size_t length, void* changes) {
        _transitions++;
        auto it = _treeStore.insert(0, TypeInstance(state, length, true));
        printf2("Report %zx\n", it.first->hash()); fflush(stdout);
        if(it.second) {
            printf2("  NEW!\n"); fflush(stdout);
            stateQueueNew.emplace_back(&*it.first);
        }
    }

    TreeStore& treeStore() { return _treeStore; }

    model* getModel() const { return _m; }

    TypeInstance const* getNextQueuedState() {
        updatable_lock lock(mtx);
        if(stateQueueNew.empty()) return nullptr;
        auto result = stateQueueNew.front();
        stateQueueNew.pop_front();
        return result;
    }

    void static reportTransitionCB(void* s, transition_info* tinfo, int* state, int* changes) {
        ssgen_mt* ss = static_cast<ssgen_mt*>(s);
        model_pins_so* model = reinterpret_cast<model_pins_so*>(ss->getModel());
        ss->reportTransition(tinfo, state, model->getLength(), changes);
    }

    virtual int pins_chunk_put(void* ctx, int type, chunk c) {
        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
        printf2("pins_chunk_put\n");
        size_t* s;
        ss->treeStore().insert(type+1, TypeInstance(c.data, c.len, true));
        printf2("pins_chunk_put %p %i %zi %i %p\n", ctx, type, *s, c.len, c.data);
        return *s;
    }

    virtual chunk pins_chunk_get(void* ctx, int type, int idx) {
        ssgen_st* ss = static_cast<ssgen_st*>(ctx);
        printf2("pins_chunk_get\n");
        auto const& ti = ss->treeStore().find(type+1, idx);
        printf2("pins_chunk_get %p %i %i %zi %p\n", ctx, type, idx, ti._length, ti._data);
        return chunk{(unsigned int)ti._length, (char*)ti._data};
    }

    virtual int pins_chunk_cam(void* ctx, int type, int idx, int offset, char* data, int len) {
        ssgen_mt* ss = static_cast<ssgen_mt*>(ctx);
        auto const& ti = ss->treeStore().find(type+1, idx);
        int newLength = (size_t)(offset + len) > ti._length ? offset + len : ti._length;
        char* copy = (char*)alloca(newLength);
        memmove(copy, ti._data, ti._length);
        memmove(copy+offset, data, len);
        auto it = ss->treeStore().insert(type+1, TypeInstance(copy, newLength, true));
        return it.first->_id;
    }

protected:
    mutex_type mtx;
    std::deque<TypeInstance const*> stateQueueNew;
    std::atomic<int> _transitions;
    TreeStore _treeStore;
};
