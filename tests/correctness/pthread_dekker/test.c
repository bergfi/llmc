#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile void llmc_assume(int b);

int x;
int y;

void* t0(void* data) {
    x = 1;
    //llmc_barrier_seq_cst();
    return (void*)(intptr_t)y;
}

void* t1(void* data) {
    y = 1;
    //llmc_barrier_seq_cst();
    return (void*)(intptr_t)x;
}

int main(int argc, char** argv) {
    pthread_t th1, th2;
    size_t R1 = 0;
    size_t R2 = 0;
    pthread_create(&th1, 0, &t0, 0);
    pthread_create(&th2, 0, &t1, 0);
    pthread_join(th1, (void**)&R1);
    pthread_join(th2, (void**)&R2);
    assert( R1 || R2  );
    return 0;
}
