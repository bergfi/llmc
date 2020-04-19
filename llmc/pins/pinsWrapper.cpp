#include <string>
#include <vector>

#include <llmc/models/PINSModel.h>
#include <llmc/ssgen.h>

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
    auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    model->setInitialState(state);
}

extern "C" void GBsetLTStype(model_t ctx_, lts_type_t ltstype) {
    auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    model->setType(ltstype);
}

extern "C" void GBsetDMInfo(model_t ctx_, matrix_t* dm) {
    auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
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
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
//    printf("ctx_:%p ctx:%p %p %p\n", ctx_, ctx, ctx->model, model);
    return model->pins_chunk_put(ctx, type, c);
}
chunk pins_chunk_get(void* ctx_, int type, int idx) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    return model->pins_chunk_get(ctx, type, idx);
}
int pins_chunk_cam(void* ctx_, int type, int idx, int offset, char* data, int len) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    return model->pins_chunk_cam(ctx, type, idx, offset, data, len);
}

__attribute__ ((visibility ("default")))  uint64_t pins_chunk_put64(void* ctx_, int type, chunk c) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    return model->pins_chunk_put64(ctx, type, c);
}
void pins_chunk_getpartial64(void* ctx_, int type, uint64_t idx, int offset, int* data, int len) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    model->pins_chunk_getpartial64(ctx, type, idx, offset, data, len);
}
chunk pins_chunk_get64(void* ctx_, int type, uint64_t idx) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    return model->pins_chunk_get64(ctx, type, idx);
}
uint64_t pins_chunk_cam64(void* ctx_, int type, uint64_t idx, int offset, int* data, int len) {
    auto ctx = static_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    return model->pins_chunk_cam64(ctx, type, idx, offset, data, len);
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
    auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    model->setGetNext(f);
}

extern "C" void GBsetNextStateAll(model_t ctx_, next_method_black_t f) {
    auto ctx = reinterpret_cast<VContext<llmc::storage::StorageInterface>*>(ctx_);
    auto model = static_cast<PINSModel*>(ctx->model);
    model->setGetNextAll(f);
}

extern "C" void dm_create(matrix_t* m, const int rows, const int cols) {
    printf("dm_create %p %i %i\n", m, rows, cols);
    m->rows = rows;
    m->cols = cols;
}
extern "C" void dm_fill(matrix_t*) {}
extern "C" lts_type_t lts_type_create() {
    return new lts_type_s;
}
extern "C" int lts_type_add_type(lts_type_t ltsType, const char* name, int* is_new) {
    size_t size = ltsType->type_db.size();
    for(size_t i = 0; i < size; ++i) {
        auto& s = ltsType->type_db[i];
        if (s == name) {
            if (is_new) *is_new = false;
            return i;
        }
    }
    if (is_new) *is_new = true;
    ltsType->type_db.push_back(name);
    return size;
}
extern "C" void lts_type_set_state_length(lts_type_t ltsType, int length) {
    printf("lts_type_set_state_length(%p, %u)\n", ltsType, length);
    ltsType->state_description.resize(length);
}
extern "C" void lts_type_set_state_typeno(lts_type_t ltsType, int stateIndex, int typeIndex) {
    ltsType->state_description.reserve(stateIndex + 1);
    ltsType->state_description[stateIndex].type = typeIndex;
}
extern "C" void lts_type_set_state_name(lts_type_t ltsType, int stateIndex, const char* name) {
    ltsType->state_description.reserve(stateIndex + 1);
    ltsType->state_description[stateIndex].name = name;
}
extern "C" void lts_type_set_edge_label_count(lts_type_t ltsType, int count) {
    printf("lts_type_set_edge_label_count(%p, %u)\n", ltsType, count);
    ltsType->edge_label.resize(count);
}
extern "C" void lts_type_set_edge_label_typeno(lts_type_t ltsType, int index, int type) {
    ltsType->edge_label.reserve(index + 1);
    ltsType->edge_label[index].type = type;
}
extern "C" void lts_type_set_edge_label_name(lts_type_t ltsType, int index, const char* name) {
    ltsType->edge_label.reserve(index + 1);
    ltsType->edge_label[index].name = name;
}
extern "C" void lts_type_set_format(lts_type_t, int, data_format_t) {}

void lts_type_destroy(lts_type_t *t) {
    delete t;
}