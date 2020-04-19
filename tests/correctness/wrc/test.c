#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile void llmc_assume(int b);

volatile int x = 0;
volatile int y = 0;
volatile int result;

void* t0(void* data) {
    x = 1;
    //llmc_barrier_seq_cst();
    return (void*)(intptr_t)y;
}

void* t1(void* data) {
	size_t R = x;
    //llmc_barrier_seq_cst();
    y = R;
    return R;
}

void* t2(void* data) {
	size_t Ry = y;
	size_t Rx = x;
    //llmc_barrier_seq_cst();
    result = (void*)((Rx << 1) | Ry);
    return (void*)((Rx << 1) | Ry);
}

int main(int argc, char** argv) {
    pthread_t th0, th1, th2;
    size_t R1 = 0;
    size_t R2 = 0;
    size_t R3 = 0;
    pthread_create(&th0, 0, &t0, 0);
    pthread_create(&th1, 0, &t1, 0);
    pthread_create(&th2, 0, &t2, 0);
    pthread_join(th0, (void**)&R1);
    pthread_join(th1, (void**)&R2);
    pthread_join(th2, (void**)&R3);
//    assert( !(R3 == 1 && R2 == 1)  );
//    assert( R3 != 1 );
    assert( result != 1 );
    return 0;
}
