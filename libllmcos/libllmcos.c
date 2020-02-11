#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>


typedef struct {
    __int64_t size;
    __int64_t start;
} __attribute((packed)) memory_info;

const int START_BYTES = 8;

void llmc_os_memory_init(void* ctx, int data_typeno, __int32_t* ci_data) {
//    printf("llmc_os_memory_init: %p %i %p\n", ctx, data_typeno, ci_data);
    chunk c_data;

    char a[START_BYTES];

    memset(a, 0, START_BYTES);

    c_data.len = START_BYTES;
    c_data.data = a;
    *ci_data = pins_chunk_put(ctx, data_typeno, c_data);

}

__int32_t llmc_os_memory_malloc(char* data, __int32_t bytes) {
//    printf("llmc_os_memory_malloc: %i %i\n", data, bytes);
    int* d = (int*)data;
    int current = *d;
    *((int*)d) += bytes;
    return current + START_BYTES;
}

void llmc_print_chunk(char* data, int len) {
    char* data_end = data + len;
    while(data < data_end) {
        printf(" %x", *data);
        data++;
    }
}

void llmc_hook___assert_fail(char* message, char* file, int line, char* x) {
    printf("ASSERT FAILED[%s:%i]: %s\n", file, line, message);
}

typedef struct {
    __uint64_t key;
    void* val;
} list_entry;

int llmc_list_find(void* list, int len, __uint64_t key, void** val) {
    list_entry* m = (list_entry*)list;
    list_entry* me = m + len/sizeof(list_entry);
//    printf("[list:%p,%i] looking for %x\n", list, len, key);
    while(m < me) {
//        printf("  - %x\n", m->key);
        if(m->key == key) {
//            printf("    -> %x\n", m->val);
            *val = m->val;
            return 1;
        }
        ++m;
    }
    return 0;
}



