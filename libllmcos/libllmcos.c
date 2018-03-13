#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>


typedef struct {
    __int64_t size;
    __int64_t start;
} __attribute((packed)) memory_info;

void llmc_os_memory_init(model_t model, int data_typeno, __int32_t* ci_data) {
    printf("llmc_os_memory_init: %p %i %p\n", model, data_typeno, ci_data);
    chunk c_data;

    const int START_BYTES = 8;

    char a[START_BYTES];

    memset(a, 0, START_BYTES);

    c_data.len = START_BYTES;
    c_data.data = a;
    *ci_data = pins_chunk_put(model, data_typeno, c_data);

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
    while(m < me) {
        if(m->key == key) {
            *val = m->val;
            return 1;
        }
        ++m;
    }
    return 0;
}




