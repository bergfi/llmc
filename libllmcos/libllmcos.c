#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>


typedef struct {
    __int64_t size;
    __int64_t start;
} __attribute((packed)) memory_info;

void llmc_os_memory_init(model_t model, int data_typeno, __int32_t* ci_data, int info_typeno, __int32_t* ci_info) {
    printf("llmc_os_memory_init: %p %i %p %i %p\n", model, data_typeno, ci_data, info_typeno, ci_info);
    chunk c_info;
    chunk c_data;
    memory_info info;

    info.size = 0;
    info.start = 16;

    c_info.len = sizeof(memory_info);
    c_info.data = (void*)&info;
    *ci_info = pins_chunk_put(model, info_typeno, c_info);

    char a[info.start];

    memset(a, 0, info.start);

    c_data.len = info.start;
    c_data.data = a;
    *ci_data = pins_chunk_put(model, data_typeno, c_data);

}

intptr_t llmc_os_malloc(int* data_len, char** p_data_data, int* info_len, char** info_data, int tid, intptr_t n) {
    printf("llmc_os_malloc: %i %p %i %p %i %" PRIiPTR "\n", *data_len, *p_data_data, *info_len, *info_data, tid, n);
    memory_info* info = (memory_info*)*info_data;
    int at = info->start + info->size;
    info->size += n;

    //char* newp = realloc(*p_data_data, info->start + info->size);
    //assert(newp);
    //memset(newp+at, 0, n);
    //*p_data_data = newp;
    *data_len = info->start + info->size;

    return at;
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
