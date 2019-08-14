#include <llmc/models/PINSModel.h>
#include <llmc/ssgen.h>

struct lts_type_s {
int state_length;
char** state_name;
int* state_type;
int state_label_count;
char** state_label_name;
int* state_label_type;
int edge_label_count;
char** edge_label_name;
int* edge_label_type;
void* type_db;
void *type_format;
int *type_min;
int *type_max;
};

#include "stdint.h" /* Replace with <stdint.h> if appropriate */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const char * data, int len) {
uint32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

extern "C" void GBsetInitialState(model_t ctx_, int* state) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    model->setInitialState(state);
}

extern "C" void GBsetLTStype(model_t ctx_, lts_type_t ltstype) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    model->setLength(ltstype->state_length*4);
}

extern "C" void GBsetDMInfo(model_t ctx_, matrix_t* dm) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    model->setTransitionGroups(dm->rows);
}

//class chunkmanager {
//public:
//    std::unordered_map<int, State> chunks;
//    int chunkidx;
//};

//std::unordered_map<int, chunkmanager> chunkmanagers;

extern "C" {

// new
int pins_chunk_put(void* ctx_, int type, chunk c) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    return model->pins_chunk_put(ctx, type, c);
}
chunk pins_chunk_get(void* ctx_, int type, int idx) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    return model->pins_chunk_get(ctx, type, idx);
}
int pins_chunk_cam(void* ctx_, int type, int idx, int offset, char* data, int len) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    return model->pins_chunk_cam(ctx, type, idx, offset, data, len);
}

//int pins_chunk_put(void* ctx_, int type, chunk c) {
//    auto mc = (ssgen*)ctx_;
//    return mc->pins_chunk_put(mc, type, c);
//}
//chunk pins_chunk_get(void* ctx_, int type, int idx) {
//    auto mc = (ssgen*)ctx_;
//    return mc->pins_chunk_get(mc, type, idx);
//}
//int pins_chunk_cam(void* ctx_, int type, int idx, int offset, char* data, int len) {
//    auto mc = (ssgen*)ctx_;
//    return mc->pins_chunk_cam(mc, type, idx, offset, data, len);
//}

//int (*pins_chunk_put)(void* ctx, int type, chunk c);
//chunk (*pins_chunk_get)(void* ctx, int type, int idx);
//int (*pins_chunk_cam)(void* ctx, int type, int idx, int offset, char* data, int len);
}

extern "C" void GBsetNextStateLong(model_t ctx_, next_method_grey_t f) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    model->setGetNext(f);
}

extern "C" void GBsetNextStateAll(model_t ctx_, next_method_black_t f) {
    MCI::ContextImpl<void, PINSModelBase>* ctx = reinterpret_cast<MCI::ContextImpl<void, PINSModelBase>*>(ctx_);
    PINSModelBase* model = reinterpret_cast<PINSModelBase*>(ctx->model);
    model->setGetNextAll(f);
}

extern "C" void dm_create(matrix_t* m, const int rows, const int cols) {
    printf("dm_create %p %i %i\n", m, rows, cols);
    m->rows = rows;
    m->cols = cols;
}
extern "C" void dm_fill(matrix_t*) {}
extern "C" lts_type_t lts_type_create() { return (lts_type_t)calloc(1, sizeof(lts_type_s)); }
extern "C" int lts_type_add_type(lts_type_t, const char*, int*) {
    static int t = 1; // used to start at 0, but 0 is now reserved for root
    return t++;
}
extern "C" void lts_type_set_state_length(lts_type_t ctx, int length) {
    ctx->state_length = length;
}
extern "C" void lts_type_set_state_typeno(lts_type_t, int, int) {}
extern "C" void lts_type_set_state_name(lts_type_t, int, const char*) {}
extern "C" void lts_type_set_edge_label_count(lts_type_t, int) {}
extern "C" void lts_type_set_edge_label_typeno(lts_type_t, int, int) {}
extern "C" void lts_type_set_edge_label_name(lts_type_t, int, const char*) {}
extern "C" void lts_type_set_format(lts_type_t, int, data_format_t) {}
