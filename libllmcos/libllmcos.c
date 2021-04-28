#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>

__attribute__((weak))
void* __LLMCOS_Object_New(size_t bytes);

__attribute__((weak))
void __LLMCOS_UnsafeOperation(int type);

__attribute__((weak))
void __LLMCOS_Object_CheckAccess(const void * ptr, size_t bytes);

__attribute__((weak))
void __LLMCOS_Object_CheckInRange(const void * ptr, const void * range_start, size_t range_bytes);

__attribute__((weak))
void __LLMCOS_Object_CheckNotOverlapping(const void* ptr1, void* ptr2, size_t bytes);

__attribute__((weak))
void __LLMCOS_Fatal(const char * assertion, const char * file, unsigned int line, const char * function);

__attribute__((weak))
void __LLMCOS_Warning(int once, const char* message);

__attribute__((weak))
int __LLMCOS_Thread_New(pthread_t* thread, void *(*start_routine) (void *), void *arg);

__attribute__((weak))
int __LLMCOS_Thread_Join(pthread_t thread, void** retval);

__attribute__((weak))
int __LLMCOS_Annotate(const char * format, ... );

__attribute__((weak))
void __LLMCOS_Assert_Fatal(int condition, const char * assertion, const char * file, unsigned int line, const char * function);

void* malloc(size_t bytes) {
    return __LLMCOS_Object_New(bytes);
}
void* calloc(size_t num, size_t size) {
    return __LLMCOS_Object_New(num * size);
}

//int memcmp ( const void * ptr1, const void * ptr2, size_t num ) {
//    __LLMCOS_Object_CheckAccess(ptr1, num);
//    __LLMCOS_Object_CheckAccess(ptr2, num);
//    const char* ptr1_ = ptr1;
//    const char* ptr2_ = ptr2;
//    const char* ptr1end = ptr1_ + num;
//    while(ptr1_ < ptr1end) {
//        char v = *ptr2_++ - *ptr1_++;
//        if(v) {
//            return v;
//        }
//    }
//    return 0;
//}

//void * memcpy ( void * destination, const void * source, size_t num ) {
//    __LLMCOS_Object_CheckAccess(destination, num);
//    __LLMCOS_Object_CheckAccess(source, num);
//    __LLMCOS_Object_CheckNotOverlapping(destination, source, num);
//    char* d = destination;
//    const char* s = source;
//    char* dend = d + num;
//    while(d < dend) {
//        *d++ = *s++;
//    }
//    return 0;
//}

//void * memmove ( void * destination, const void * source, size_t num ) {
//    __LLMCOS_Object_CheckAccess(destination, num);
//    __LLMCOS_Object_CheckAccess(source, num);
//    char* d = destination;
//    const char* s = source;
//    char* dend = d + num;
//    if(d < s) {
//        while(d < dend) {
//            *d++ = *s++;
//        }
//    } else {
//        const char* send = s + num;
//        while(d < dend) {
//            *dend-- = *send--;
//        }
//
//    }
//    return 0;
//}

void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function) {
    __LLMCOS_Fatal(assertion, file, line, function);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
    if(attr) {
        __LLMCOS_Warning(1, "pthread_create called with attr, ignoring");
    }
    return __LLMCOS_Thread_New(thread, start_routine, arg);
}

int pthread_join(pthread_t thread, void **retval) {
    // TODO: implement
    return __LLMCOS_Thread_Join(thread, retval);
}

int printf ( const char * format, ... ) {
    va_list argptr;
    va_start(argptr,format);
    int r = __LLMCOS_Annotate(format, argptr);
    va_end(argptr);
    return r;
}
