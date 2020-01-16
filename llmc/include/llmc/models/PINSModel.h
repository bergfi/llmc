#pragma once

#include <libfrugi/Settings.h>
#include <llmc/modelcheckers/interface.h>
#include <llmc/model.h>
#include <llmc/pins/pins.h>

struct lts_label {
    std::string name;
    int type;
};

struct lts_type_s {
    std::vector<lts_label> state_description;
    std::vector<lts_label> state_label;
    std::vector<lts_label> edge_label;
    std::vector<std::string> type_db;
};

class PINSModelBase {
public:
    typedef void(*model_init_t)(void* model);
    //typedef int(*next_method_grey_t)(void* self,int, void*src,void* cb,void*user_context);
    //typedef int(*next_method_black_t)(void* self,void*src,void* cb,void*user_context);
public:
    PINSModelBase(void* handle): _init(nullptr), _handle(handle) {
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

    void setType(lts_type_s* ltsType) {
        _ltsType = ltsType;
        printf("SET LENGTH %zu\n", ltsType->state_description.size()); fflush(stdout);
        _length = ltsType->state_description.size();

        for(size_t i = 0; i < ltsType->edge_label.size(); ++i) {
            auto& label = ltsType->edge_label[i];
            if(label.name == "action") {
                _labelIndex = i;
            }
        }
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
    lts_type_s* _ltsType;
    int _length;
    int _tgroups;
    uint32_t* _init;
    model_init_t _pins_model_init;
    next_method_grey_t _pins_getnext;
    next_method_black_t _pins_getnextall;
    void* _handle;
    TransitionCB _reportTransition_cb;
    std::string _labelName = "action";
    size_t _labelIndex = -1;
};

class PINSModel: public PINSModelBase, public VModel<llmc::storage::StorageInterface> {
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

//    struct VPINSTransitionInfo: public VTransitionInfo {
//        TransitionInfo operator()(typename MODELCHECKER::Context* ctx) const {
//            return getTransitionInfo(ctx, pinsTransitionInfo);
//        }
//
//        VPINSTransitionInfo(const transition_info* pinsTransitionInfo): pinsTransitionInfo(pinsTransitionInfo) {
//        }
//
//        const transition_info* pinsTransitionInfo;
//    };

//    struct PINSTransitionInfo: public TransitionInfoUnExpanded {
//        TransitionInfo operator()(typename MODELCHECKER::Context* ctx) const {
//            return getTransitionInfo(ctx, pinsTransitionInfo);
//        }
//
//        VPINSTransitionInfo(const transition_info* pinsTransitionInfo): pinsTransitionInfo(pinsTransitionInfo) {
//        }
//
//        const transition_info* pinsTransitionInfo;
//    };

    PINSModel(void* handle): PINSModelBase(handle) {
    }

    ~PINSModel() {
    }

    size_t getNextAll(StateID const& s, Context* ctx) override {

        printf("getNextAll(%x,%p)\n", s.getData(), ctx);

        //const typename MODELCHECKER::FullState* state = ctx->getModelChecker()->getState(ctx, s);
        //_pins_getnextall();
        auto cb = (TransitionCB)reportTransitionCB;//getCB_ReportTransition();
        for(int tg = 0; tg < this->_tgroups; ++tg) {
//            _pins_getnext((model_t)nullptr, tg, ((int*)nullptr), nullptr, (void *)nullptr);
            auto fsd = ctx->getModelChecker()->getState(ctx, s);
            _pins_getnext((model_t)ctx, tg, ((int*)fsd->getData()), cb, (void *)ctx);
        }
        return 0;
    }
    StateID getInitial(Context* ctx) override {
        assert(_pins_model_init);
        _pins_model_init((void*)ctx);
        assert(this->_init);
        return ctx->getModelChecker()->newState(0, _length, _init).getState();
    }

    static llmc::storage::StorageInterface::StateTypeID getTypeID(int type) {
        return type;
    }

    virtual int pins_chunk_put(void* ctx_, int type, chunk c) {
        auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
        auto r = ctx->mc->newState(getTypeID(type), (size_t)(c.len / sizeof(llmc::storage::StorageInterface::StateSlot)), (llmc::storage::StorageInterface::StateSlot*)c.data).getState().getData();
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
        auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
        chunk c;
        auto fsd = ctx->mc->getSubState(ctx, idx);
        c.len = fsd->getLength() * sizeof(llmc::storage::StorageInterface::StateSlot);
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
        auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
        auto d = ctx->mc->newDelta(offset, (llmc::storage::StorageInterface::StateSlot*)data, len / sizeof(llmc::storage::StorageInterface::StateSlot*));
        auto s = ctx->mc->newSubState(idx, *d);
        ctx->mc->deleteDelta(d);
        return s.getData();
    }

    static void reportTransitionCB(void* ctx_, transition_info* tinfo, int* state, int* changes) {
        auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
        TransitionInfoUnExpanded t = TransitionInfoUnExpanded::construct(tinfo);
        ctx->mc->newTransition( ctx
                              , ((PINSModel*)ctx->model)->getLength()
                              , (llmc::storage::StorageInterface::StateSlot*)state
                              , t
                              );
    }

    TransitionInfo getTransitionInfo(VContext<llmc::storage::StorageInterface>* ctx, TransitionInfoUnExpanded const& tinfo_) const override {
        auto tinfo = tinfo_.get<transition_info*>();
        if(tinfo && tinfo->labels) {
            auto labelIndex = ctx->getModel<PINSModel>()->_labelIndex;
            if(labelIndex < ctx->getModel<PINSModel>()->_ltsType->edge_label.size()) {
                auto fsd = ctx->mc->getSubState(ctx, tinfo->labels[labelIndex]);
                return TransitionInfo(std::string(fsd->getCharData(), fsd->getLengthInBytes()));
            }
        }
        return TransitionInfo();
    }

    llmc::statespace::Type* getStateVectorType() override {
        return nullptr;
    }

protected:
    typename llmc::storage::StorageInterface::StateID _initID;
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
