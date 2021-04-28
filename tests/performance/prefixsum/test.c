#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef DATALENGTH
#define DATALENGTH 1
#endif
#ifndef THREADS
#define THREADS 1
#endif

int in[] = {
0,1,2,3,4,5,6,7,8,9,
10,11,12,13,14,15,16,17,18,19,
20,21,22,23,24,25,26,27,28,29,
30,31,32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,48,49,
50,51,52,53,54,55,56,57,58,59,
60,61,62,63,64,65,66,67,68,69,
70,71,72,73,74,75,76,77,78,79,
80,81,82,83,84,85,86,87,88,89,
};
int out[DATALENGTH];

typedef struct {
	int tid;
	int* start;
	int* end;
} Region;

intptr_t sums[THREADS];
pthread_t th[THREADS];
#ifdef REGIONS_GLOBAL
	Region* regions[THREADS];
#endif

void* localprefixsum(void* data) {
	Region* region = (Region*)data;
	int* start = region->start;
	int* end = region->end;
	int* pOut = out + (start - in);
	intptr_t sum = 0;
	while(start < end) {

		int d = __atomic_load_n(start, __ATOMIC_SEQ_CST);
		sum += d;
		start++;
//		sum += *start++;

		__atomic_store_n(pOut, sum, __ATOMIC_SEQ_CST);
		pOut++;
//		*pOut++ = sum;

	}

	//	sums[(intptr_t)data] = sum;
	__atomic_store_n(sums+region->tid, sum, __ATOMIC_SEQ_CST);

	return NULL;
}

void* localadd(void* data) {
	Region* region = (Region*)data;
	int* end = out + (region->end - in);
	int* pOut = out + (region->start - in);

	//	int sum = sums[(intptr_t)data-1];
	int sum = __atomic_load_n(sums+region->tid-1, __ATOMIC_SEQ_CST);

	while(pOut < end) {
		int s = __atomic_load_n(pOut, __ATOMIC_SEQ_CST);
//		int s = *pOut;

		__atomic_store_n(pOut, s + sum, __ATOMIC_SEQ_CST);
		pOut++;
//		*pOut++ += sum;
	}
	return NULL;
}

void inlineprefixsum(intptr_t* start, intptr_t* end) {
	intptr_t sum = 0;
	while(start < end) {
//		int s = __atomic_load_n(start, __ATOMIC_SEQ_CST);
//		sum += s;
//		__atomic_store_n(start, sum, __ATOMIC_SEQ_CST);
//		start++;
		sum += *start;
		*start++ = sum;
	}
}

int main(int argc, char** argv) {

#ifndef REGIONS_GLOBAL
	Region* regions[THREADS];
#endif

	double perThread = (double)DATALENGTH / THREADS;
	double current = 0;

	regions[0] = (Region*)malloc(sizeof(Region));
	regions[0]->start = in;
	regions[0]->tid = 0;
	for(int t = 1; t < THREADS; ++t) {
		current += perThread;
		regions[t] = (Region*)malloc(sizeof(Region));
		regions[t]->tid = t;
		regions[t-1]->end = in + (size_t)current;
		regions[t]->start = in + (size_t)current;
	}
	regions[THREADS-1]->end = in + DATALENGTH;

	for(int t = 1; t < THREADS; ++t) {
	    pthread_create(&th[t], 0, &localprefixsum, (void*)regions[t]);
	}
    localprefixsum(regions[0]);

	for(int t = 1; t < THREADS; ++t) {
	    pthread_join(th[t], NULL);
	}

	inlineprefixsum(&sums[0], &sums[THREADS]);

	for(int t = 1; t < THREADS; ++t) {
	    pthread_create(&th[t], 0, &localadd, (void*)regions[t]);
	}
	for(int t = 1; t < THREADS; ++t) {
	    pthread_join(th[t], NULL);
	}

	assert(out[DATALENGTH-1] == DATALENGTH*(DATALENGTH-1)/2);
    return 0;
}
