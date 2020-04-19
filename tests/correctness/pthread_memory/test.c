#include <assert.h>
#include <stdint.h>
#include <pthread.h>

void* t0(void* data) {
	int a = 1234;
    return (void*)(intptr_t)a;
}

void* t1(void* data) {
	int a = 5678;
    return (void*)(intptr_t)a;
}

int main(int argc, char** argv) {
    pthread_t th1, th2;
    size_t R1 = 0, R2 = 0;
    pthread_create(&th1, 0, &t0, 0);
    pthread_create(&th2, 0, &t1, 0);
    pthread_join(th1, (void**)&R1);
    pthread_join(th2, (void**)&R2);
    assert( R1 == 1234 );
    assert( R2 == 5678 );
    return 0;
}
