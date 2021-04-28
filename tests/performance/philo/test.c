#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef THREADS
#define THREADS 4
#endif
#ifndef RESOURCEREQUESTS
#define RESOURCEREQUESTS 12
#endif

int verify();

size_t forks[THREADS];
size_t times[THREADS];
pthread_t th[THREADS];

void* philo(void* _id) {
	uintptr_t id = (uintptr_t)_id;
	uintptr_t leftFork = id > 0 ? id - 1 : THREADS - 1;
	uintptr_t rightFork = id;
	size_t expectedForkValue = 0;
	times[id] = 0;
	while(times[id] < RESOURCEREQUESTS) {
    	//think until the left fork is available; when it is, pick it up;
		while(!__atomic_compare_exchange_n(&forks[leftFork], &expectedForkValue, id, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
			expectedForkValue = 0;
		}

    	//think until the right fork is available; when it is, pick it up;
		while(!__atomic_compare_exchange_n(&forks[rightFork], &expectedForkValue, id, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
			expectedForkValue = 0;
		}

    	// when both forks are held, eat for a fixed amount of time;

		// then, put the right fork down;
		__atomic_store_n(&forks[rightFork], 0, __ATOMIC_SEQ_CST);

    	// then, put the left fork down;
		__atomic_store_n(&forks[leftFork], 0, __ATOMIC_SEQ_CST);

    	// repeat from the beginning.
		//times[id]++;
		size_t v = __atomic_load_n(&times[id], __ATOMIC_SEQ_CST);
		__atomic_store_n(&times[id], v+1, __ATOMIC_SEQ_CST);
	}
	return NULL;
}

int main(int argc, char** argv) {

	for(int t = 1; t < THREADS; ++t) {
	    pthread_create(&th[t], 0, &philo, (void*)(intptr_t)t);
	}
	int failed = (intptr_t)philo(0);

	for(int t = 1; t < THREADS; ++t) {
	    pthread_join(th[t], NULL);
	}

	return verify();
}

int verify() {
	for(int t = 0; t < THREADS; ++t) {
		assert(forks[t] == 0);
		assert(times[t] == RESOURCEREQUESTS);
//		printf("times[%u] = %zu\n", t, times[t]);
	}
	return 0;
}
