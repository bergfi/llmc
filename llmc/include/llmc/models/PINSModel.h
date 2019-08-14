#pragma once

#include <libfrugi/Settings.h>
#include <llmc/modelcheckers/interface.h>
#include <llmc/model.h>
#include <llmc/pins/pins.h>

class PINSModelBase {
public:
    typedef void(*model_init_t)(void* model);
    //typedef int(*next_method_grey_t)(void* self,int, void*src,void* cb,void*user_context);
    //typedef int(*next_method_black_t)(void* self,void*src,void* cb,void*user_context);
public:
    PINSModelBase(void* handle): _init(nullptr), _handle(handle) {
        std::cout << std::endl << "PINSModel() " << &_init << ", " << _init << std::endl;
        _pins_model_init = reinterpret_cast<decltype(_pins_model_init)>(dlsym(_handle, "pins_model_init"));
        //_pins_getnextall = reinterpret_cast<decltype(_pins_getnextall)>(dlsym(_handle, "pins_getnextall"));
        assert(!_init);
    }

    ~PINSModelBase() {
        std::cout << "~PINSModel()" << std::endl;
        if(_init) {
            free(_init);
        }
        assert(0);
    }

    void setGetNext(next_method_grey_t f) {
        _pins_getnext = f;
    }

    void setGetNextAll(next_method_black_t f) {
        _pins_getnextall = f;
    }

    void setInitialState(int* s) {
        //int* data = (int*)malloc(sizeof(int) + _length);
        //assert(data);
        //*data = _length;
        //memmove(data+1, s, _length);
        assert(_length);
        assert(s);
        //_init = std::move(TypeInstance(s, _length, true));

        assert(!_init);
        _init = (uint32_t*)realloc((void*)_init, _length * sizeof(uint32_t));
        memcpy(_init, s, _length * sizeof(uint32_t));

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

    void setCB_ReportTransition(TransitionCB cb) {
        _reportTransition_cb = cb;
    }

    virtual int pins_chunk_put(void* ctx_, int type, chunk c) = 0;
    virtual chunk pins_chunk_get(void* ctx_, int type, int idx) = 0;
    virtual int pins_chunk_cam(void* ctx_, int type, int idx, int offset, char* data, int len) = 0;

    TransitionCB getCB_ReportTransition() {
        return _reportTransition_cb;
    }

protected:
    int _length;
    int _tgroups;
    uint32_t* _init;
    model_init_t _pins_model_init;
    next_method_grey_t _pins_getnext;
    next_method_black_t _pins_getnextall;
    void* _handle;
    TransitionCB _reportTransition_cb;
};

template<typename MODELCHECKER>
class PINSModel: public PINSModelBase, Model<MODELCHECKER> {
public:
    static PINSModel* get(std::string const& filename) {
        auto handle = dlopen(filename.c_str(), RTLD_LAZY);
        if(!handle) {
            printf("Failed to open %s: %s\n", filename.c_str(), dlerror());
            assert(0);
            return nullptr;
        }
        static int times = 0;
        times++;
        assert(times==1);
        return new PINSModel(handle);
    }

public:

    PINSModel(void* handle): PINSModelBase(handle) {
    }

    ~PINSModel() {
    }

    size_t getNextAll(typename MODELCHECKER::StateID const& s, typename MODELCHECKER::Context& ctx) {

        const typename MODELCHECKER::FullState* state = ctx.mc->getState(&ctx, s);
        //_pins_getnextall();
        auto cb = (TransitionCB)reportTransitionCB;//getCB_ReportTransition();
        for(int tg = 0; tg < this->_tgroups; ++tg) {
//            _pins_getnext((model_t)nullptr, tg, ((int*)nullptr), nullptr, (void *)nullptr);
            auto fsd = ctx.mc->getState(&ctx, s);
            _pins_getnext((model_t)&ctx, tg, ((int*)fsd->getData()), cb, (void *)&ctx);
        }
        return 0;
    }
    typename MODELCHECKER::StateID getInitial(typename MODELCHECKER::Context& ctx) {
        assert(_pins_model_init);
        _pins_model_init((void*)&ctx);
        assert(this->_init);
        return ctx.mc->newState(0, _length, _init).getState();
    }

    static typename MODELCHECKER::StateTypeID getTypeID(int type) {
        return type;
    }

    virtual int pins_chunk_put(void* ctx_, int type, chunk c) {
        typename MODELCHECKER::Context* ctx = reinterpret_cast<typename MODELCHECKER::Context*>(ctx_);
        auto r = ctx->mc->newState(getTypeID(type), (size_t)(c.len / sizeof(typename MODELCHECKER::StateSlot)), (typename MODELCHECKER::StateSlot*)c.data).getState().getData();
//        printf("pins_chunk_put %u:", c.len);
//        char* ch = c.data;
//        char* end = c.data + c.len;
//        while(ch < end) {
//            printf(" %02x", *ch);
//            ch++;
//        }
//        printf("\n");
        return r;
    }
    virtual chunk pins_chunk_get(void* ctx_, int type, int idx) {
        typename MODELCHECKER::Context* ctx = reinterpret_cast<typename MODELCHECKER::Context*>(ctx_);
        chunk c;
        auto fsd = ctx->mc->getSubState(ctx, idx);
        c.len = fsd->getLength() * sizeof(typename MODELCHECKER::StateSlot);
        c.data = (decltype(c.data))fsd->getData();
//        printf("pins_chunk_get %u:", c.len);
//        char* ch = c.data;
//        char* end = c.data + c.len;
//        while(ch < end) {
//            printf(" %02x", *ch);
//            ch++;
//        }
//        printf("\n");
        return c;
    }
    virtual int pins_chunk_cam(void* ctx_, int type, int idx, int offset, char* data, int len) {
        typename MODELCHECKER::Context* ctx = reinterpret_cast<typename MODELCHECKER::Context*>(ctx_);
        auto d = ctx->mc->newDelta(offset, (typename MODELCHECKER::StateSlot*)data, len / sizeof(typename MODELCHECKER::StateSlot*));
        auto s = ctx->mc->newSubState(idx, *d);
        ctx->mc->deleteDelta(d);
        return s.getData();
    }

    static void reportTransitionCB(void* ctx_, transition_info* tinfo, int* state, int* changes) {
        typename MODELCHECKER::Context* ctx = reinterpret_cast<typename MODELCHECKER::Context*>(ctx_);
        ctx->mc->newTransition(ctx, ((PINSModel*)ctx->model)->getLength(), (typename MODELCHECKER::StateSlot*)state);
    }

protected:
    typename MODELCHECKER::StateID _initID;
};

//template<typename MODELCHECKER>
//class PINSModelCheckerWrapper {
//public:
//
//    PINSModelCheckerWrapper(MODELCHECKER* m): _m(m) {
//
//    }
//
//    int pins_chunk_put(int type, chunk c) {
//        return 0;
//    }
//    chunk pins_chunk_get(int type, int idx) {
//        return {0};
//    }
//    int pins_chunk_cam(int type, int idx, int offset, char* data, int len) {
//        return 0;
//    }
//
//    void reportTransition(void* tinfo, void* state, size_t length, void* changes) {
//        //_m->newTransition(
//    }
//
//    static void reportTransitionCB(void* s, transition_info* tinfo, int* state, int* changes) {
////        PINSModelCheckerWrapper* ss = reinterpret_cast<PINSModelCheckerWrapper*>(ctx);
////        ss->reportTransition(tinfo, state, model->getLength(), changes);
//    }
//
//protected:
//    MODELCHECKER* _m;
//};
