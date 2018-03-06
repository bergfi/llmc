#define pthread_t int

extern void pthread_create(pthread_t* t, int a, void* d, int b);
extern void pthread_join(pthread_t* t, void* r);
extern void pthread_assert(int c);

volatile void llmc_assume(int b);

int x;
int y;

void* t0(void* data) {
	x = 1;
	//llmc_barrier_seq_cst();
	return y;
}

void* t1(void* data) {
	y = 1;
	//llmc_barrier_seq_cst();
	return x;
}

int main(int argc, char** argv) {
	pthread_t th1, th2;
	int R1;
	int R2;
	pthread_create(&th1, 0, &t0, 0);
	pthread_create(&th2, 0, &t1, 0);
	pthread_join(th1, &R1);
	pthread_join(th2, &R2);
	assert( R1 || R2  );
	return 0;
}
