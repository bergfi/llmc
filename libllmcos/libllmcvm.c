#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ltsmin/pins.h>
//#include <ltsmin/pins-util.h>


typedef struct {
    __int64_t size;
    __int64_t start;
} __attribute((packed)) memory_info;

const int START_BYTES = 8;

__int32_t llmc_os_memory_malloc(char* data, __int32_t bytes);

//void llmc_os_memory_init(void* ctx, int data_typeno, __int64_t* ci_data, __int32_t globalBytes) {
//    chunk c_data;
//
//    char a[START_BYTES];
//    memset(a, 0, START_BYTES);
//    llmc_os_memory_malloc(a, globalBytes);
//
//    c_data.len = START_BYTES;
//    c_data.data = a;
//    *ci_data = pins_chunk_put64(ctx, data_typeno, c_data);
////    printf("[LLMC OS] Initialized memory to %u bytes (chunk ID: %zx)\n", c_data.len, *ci_data);
//    fflush(stdout);
//}

//__int32_t llmc_os_memory_malloc(char* data, __int32_t bytes) {
//    int* d = (int*)data;
//    int current = *d;
//    *((int*)d) += bytes;
////    printf("[LLMC OS] Allocated %u bytes @ %x\n", bytes, current + START_BYTES);
//    return current + START_BYTES;
//}

void llmc_print_chunk(char* data, int len) {
    char* data_end = data + len;
    while(data < data_end) {
//        printf(" %x", *data);
        data++;
    }
}

//void llmc_hook___assert_fail(char* message, char* file, int line, char* x) {
//    printf("[LLMC OS] ASSERT FAILED[%s:%i]: %s\n", file, line, message);
//}

//void llmc_hook_printf(char* str, ...) {
//    va_list(args);
//    va_start(args, str);
//    vprintf(str, args);
//}

//void llmc_hook_fflush(FILE* file) {
//}

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
            if(val) *val = m->val;
            return 1;
        }
        ++m;
    }
    return 0;
}

//int llmc_memcmp(char* one, char* other, size_t size) {
//    char* oneI = one;
//    char* oneE = one + size;
//    printf("one:   ");
//    while(oneI < oneE) {
//        printf(" %x", *oneI);
//        oneI++;
//    }
//    printf("\n");
//    char* otherI = other;
//    char* otherE = other + size;
//    printf("other: ");
//    while(otherI < otherE) {
//        printf(" %x", *otherI);
//        otherI++;
//    }
//    printf("\n");
//    int r = memcmp(one, other, size);
//    printf("r: %zu", size);
//    return r;
//}

int llmc_memory_check(size_t memAccess) {
    if(!memAccess) {
        abort();
    }
    return 0;
}

int __LLMCOS_Annotate(const char * format, ... ) {
    va_list argptr;
    va_start(argptr,format);
    int r = printf(format, argptr);
    va_end(argptr);
    return r;
}

void __LLMCOS_Fatal(const char * assertion, const char * file, unsigned int line, const char * function) {
    printf("FATAL: %s:%u: %s (%s)\n", file, line, assertion, function);
    abort();
}

void __LLMCOS_Assert_Fatal(int condition, const char * assertion, const char * file, unsigned int line, const char * function) {
    if(!condition) {
        printf("FATAL: %s:%u: %s (%s)\n", file, line, assertion, function);
        abort();
    }
}

void __LLMCOS_CheckRangeIsInRange(const void * ptr, size_t ptr_size, const void * range_start, size_t range_bytes) {
//    fprintf(stderr, "Range check (%p, %zu): %p + %zu\n", range_start, range_bytes, ptr, ptr_size);
    if(ptr < range_start || (char*)ptr + ptr_size > (char*)range_start + range_bytes) {
        fprintf(stderr, "Out of range (%p, %zu): %p + %zu\n", range_start, range_bytes, ptr, ptr_size);
        fflush(stderr);
        abort();
    }
}

int __LLMCOS_memcmp(const void* s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n);
}