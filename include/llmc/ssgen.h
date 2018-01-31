#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
extern "C" {
#include <ltsmin/pins.h>
}

uint32_t SuperFastHash (const char * data, int len);

class ssgen;

class State {
public:

    bool operator==(State const& other) const {
        if(this == &other) return true;
        auto thisint_p = reinterpret_cast<int*>(this->_data);
        auto otherint_p = reinterpret_cast<int*>(this->_data);
        if(*thisint_p != *otherint_p) return false;
        return memcmp(this->_data, other._data, *thisint_p);
    }

    State(): _data(nullptr) {}
    State(void* data): _data(data) {}
    ~State() {
        if(_data) free(_data);
    }

    State(State const& other) = delete;
    State(State&& other) {
        this->_data = other._data;
        other._data = nullptr;
    }

    State const& operator=(State const& other) = delete;
    State const& operator=(State&& other) {
        this->_data = other._data;
        other._data = nullptr;
        return *this;
    }

    static State clone(void* data) {
        auto length = *reinterpret_cast<int*>(data);
        void* dataClone = malloc(sizeof(int) + length);
        memmove(dataClone, data, sizeof(int) + length);
        return std::move(State(dataClone));
    }

    static State clone(int length, void* data) {
        int* dataClone = (int*)malloc(sizeof(int) + length);
        *dataClone = length;
        memmove(dataClone+1, data, length);
        return std::move(State(dataClone));
    }

    uint32_t hash() const {
        int* thisint_p = reinterpret_cast<int*>(_data);
        return SuperFastHash(reinterpret_cast<char*>(_data), *thisint_p);
    }

    void* const& data() const { return _data; }

    State clone() {
        return std::move(State::clone(this->_data));
    }

private:
    void* _data;
};

class model {
public:
    static model* getPINS(std::string const& filename);
    virtual int getNextAll(State const& s, ssgen* ss) = 0;
    virtual void getInitial(State& s) = 0;
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
        if(!handle) return nullptr;
        return new model_pins_so(handle);
    }

    int getNextAll(State const& s, ssgen* ss);

    void setInitialState(int* s) {
        int* data = (int*)malloc(sizeof(int) + _length);
        *data = _length;
        memmove(data+1, s, _length);
        _init = std::move(State(data));
        printf("SET INITIAL %x\n", _init.hash()); fflush(stdout);
    }

    void getInitial(State& s) {
        _pins_model_init((void*)this);
        s = _init.clone();
    }

    void setLength(int length) {
        printf("SET LENGTH %i\n", length); fflush(stdout);
        _length = length;
    }

    int getLength() const { return _length; }

    void setTransitionGroups(int tgroups) {
        printf("SET TGROUPS %i\n", tgroups); fflush(stdout);
        _tgroups = tgroups;
    }

    static void _reportTransition_cb(void* s, void* tinfo, void* state, void* changes);

private:
    int _length;
    int _tgroups;
    State _init;
    model_init_t _pins_model_init;
    next_method_grey_t _pins_getnext;
    next_method_black_t _pins_getnextall;
    void* _handle;
};

namespace std {
template <>
struct hash<State>
{
    std::size_t operator()(State const& state) const {
        return state.hash();
    }
};
}

class ssgen {
public:
    ssgen(model* m): _m(m) {
    }

    void go() {
        State init;
        _m->getInitial(init);
        auto it = stateSpace.insert(State::clone(init.data()));
        stateQueue.emplace_back(&*it.first);
        do {
            State const* s = stateQueue.front();
            printf("Go %x\n", s->hash()); fflush(stdout);
            stateQueue.pop_front();
            _m->getNextAll(*s, this);
        } while(!stateQueue.empty());
        printf("Stopped\n"); fflush(stdout);
        printf("Found %zu states\n", stateSpace.size());
    }

    void reportTransition(void* tinfo, void* state, void* changes) {
        auto it = stateSpace.insert(State(state));
        printf("Report %x\n", it.first->hash()); fflush(stdout);
        if(it.second) {
            printf("  NEW!\n"); fflush(stdout);
            stateQueue.emplace_back(&*it.first);
        }
    }

    model* getModel() const { return _m; }

private:
    model* _m;
    std::deque<State const*> stateQueue;
    std::unordered_set<State> stateSpace;
};
