#include <assert.h>
#include <stdint.h>
#include <pthread.h>

pthread_t gth1;
pthread_t gth2;

void* tleaf(void* data) {
    return (void*)(intptr_t)1234;
}

void* t0(void* data) {
	pthread_create((pthread_t*)data, 0, &tleaf, 0);
    return 0;//(void*)(intptr_t)th;
}

int main(int argc, char** argv) {
    pthread_t th1, th2;
    pthread_t th1r, th2r;
	size_t R1 = 0, R2 = 0;
    pthread_create(&th1, 0, &t0, (void*)&gth1);
    pthread_create(&th2, 0, &t0, (void*)&gth2);
//    pthread_join(th1, (void**)&th1r);
//    pthread_join(th2, (void**)&th2r);
    pthread_join(th1, 0);
    pthread_join(th2, 0);
    pthread_join(gth1, (void**)&R1);
    pthread_join(gth2, (void**)&R2);
    assert( R1 == 1234 && R2 == 1234 );
	gth1 = gth2 = 0;
    return 0;
}
