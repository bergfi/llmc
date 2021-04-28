#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef DATALENGTH
#define DATALENGTH 16
#endif
#ifndef THREADS
#define THREADS 4
#endif
#ifndef HASHMAPSIZE
#define HASHMAPSIZE 16
#endif

int verify(int failed);

#ifdef NDUPLICATES
size_t input[] = {
3              , HASHMAPSIZE+3,             7, HASHMAPSIZE*2+3,
1              ,             5, HASHMAPSIZE+7,               2,
HASHMAPSIZE+1  ,             4, HASHMAPSIZE+5, HASHMAPSIZE  +2,
HASHMAPSIZE*2+7,             6, HASHMAPSIZE+4,               8,
};
#else
size_t input[] = {
3              , HASHMAPSIZE+3,             7,               3,
1              , HASHMAPSIZE+7,             7,               2,
HASHMAPSIZE+1  ,             2, HASHMAPSIZE+4, HASHMAPSIZE  +2,
HASHMAPSIZE*2+7,             6,             4,               8,
};
#endif

size_t map[HASHMAPSIZE*2];

pthread_t th[THREADS];

size_t hash(size_t e) {
	return e;
}

size_t findorput(size_t key, size_t value) {
	
	size_t index = hash(key) % HASHMAPSIZE;
	
	size_t* k = map + 2 * index;
	size_t* kend = map + 2 * HASHMAPSIZE;
	for(;;) {
		size_t oldkValue = __atomic_load_n(k, __ATOMIC_SEQ_CST);
		while(oldkValue == 0) {
			if(__atomic_compare_exchange_n(k, &oldkValue, key, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
				__atomic_store_n(k+1, value, __ATOMIC_SEQ_CST);
				return 1;
			}
		}
		if(oldkValue == key) {
			size_t v;
			do {
				v = __atomic_load_n(k+1, __ATOMIC_SEQ_CST);
			} while(!v);
			return 0;
		}
		k += 2;
		if(k >= kend) {
			k = map;
		}
	}
}

void* adder(void* data) {
	intptr_t tid = (intptr_t)data;
	size_t* in = input + (DATALENGTH/THREADS) * tid;
	size_t* end = in + (DATALENGTH/THREADS);
	int failed = 0;
	while(in < end) {
		failed += !findorput(*in++, 1);
	}
	return (void*)(intptr_t)failed;
}

int main(int argc, char** argv) {

	for(int t = 1; t < THREADS; ++t) {
	    pthread_create(&th[t], 0, &adder, (void*)(intptr_t)t);
	}
	int failed = (intptr_t)adder(0);

	for(int t = 1; t < THREADS; ++t) {
		intptr_t f;
	    pthread_join(th[t], (void*)&f);
// 		printf("f: %zu\n", f);
		failed += f;
	}

// 	printf("failed: %zu\n", failed);
	return verify(failed);
}

int verify(int failed) {
	size_t* k = map;
	size_t* kend = map + 2 * HASHMAPSIZE;
	int count = 0;
	while(k < kend) {
		if(*k) {
// 			printf("k: %zu\n", *k);
			count++;
		}
		k += 2;
	}

//  	if(count + failed != DATALENGTH) {
//  		printf("count: %u, failed: %u, DATALENGTH: %u\n", count, failed, DATALENGTH);
//  	}
	
#ifdef RESULTCHECK
	printf(
		"RESULTCHECK"
		" %zu:%zu %zu:%zu %zu:%zu %zu:%zu %zu:%zu %zu:%zu"
#if HASHMAPSIZE > 6
		" %zu:%zu"
#endif		
#if HASHMAPSIZE > 7
		" %zu:%zu"
#endif		
#if HASHMAPSIZE > 8
		" %zu:%zu"
#endif		
#if HASHMAPSIZE > 9
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 10
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 11
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 12
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 13
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 14
		" %zu:%zu"
#endif
#if HASHMAPSIZE > 15
		" %zu:%zu"
#endif
		"\n"
	, map[0], map[1], map[2], map[3], map[4], map[5], map[6], map[7], map[8], map[9], map[10], map[11]
#if HASHMAPSIZE > 6
	,	map[12], map[13]
#endif
#if HASHMAPSIZE > 7
	,	map[14], map[15]
#endif		
#if HASHMAPSIZE > 8
	,	map[16], map[17]
#endif
#if HASHMAPSIZE > 9
	,	map[18], map[19]
#endif
#if HASHMAPSIZE > 10
	,	map[20], map[21]
#endif
#if HASHMAPSIZE > 11
	,	map[22], map[23]
#endif
#if HASHMAPSIZE > 12
	,	map[24], map[25]
#endif
#if HASHMAPSIZE > 13
	,	map[26], map[27]
#endif
#if HASHMAPSIZE > 14
	,	map[28], map[29]
#endif
#if HASHMAPSIZE > 15
	,	map[30], map[31]
#endif
	);
#endif
	assert(count + failed == DATALENGTH);

	return 0;
}
