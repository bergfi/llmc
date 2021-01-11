#include <assert.h>
#include <stdint.h>
#include <pthread.h>

void* t0(void* data) {
    return (void*)(intptr_t)1234;
}

int main(int argc, char** argv) {
    pthread_t th1;
    size_t R1 = 0;
	assert(&th1);
//	assert(&th1 == (pthread_t*)0x0100000000000000ULL);
    pthread_create(&th1, 0, &t0, 0);
    pthread_join(th1, (void**)&R1);
    assert( R1 == 1234 );
    return 0;
}
