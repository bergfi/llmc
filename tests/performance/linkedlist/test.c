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

int verify(int failed);

intptr_t input[] = {
90,77,3,5,89,9,6,56,34,12,
2,67,17,31,57,96,73,33,93,99,
7,76,53,13,98,23,71,78,51,10,
1,40,15,61,47,66,88,11,83,43
};

typedef struct Node {
	intptr_t data;
	struct Node* next;
} Node;

Node* root = NULL;

pthread_t th[THREADS];

int addnode(intptr_t data) {
	Node* node = (Node*)malloc(sizeof(Node));
	if(!node) {
		return 1;
	}
	node->data = data;
	Node** target = &root;
	node->next = __atomic_load_n(target, __ATOMIC_SEQ_CST);
	while(!__atomic_compare_exchange_n(target, &node->next, node, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
	}
	return 0;
}

void* adder(void* data) {
	intptr_t tid = (intptr_t)data;
	intptr_t* in = input + (DATALENGTH/THREADS) * tid;
	intptr_t* end = in + (DATALENGTH/THREADS);
	int failed = 0;
	while(in < end) {
		failed += addnode(*in++);
	}
	return (void*)(intptr_t)failed;
}

int main(int argc, char** argv) {

	for(int t = 1; t < THREADS; ++t) {
	    pthread_create(&th[t], 0, &adder, (void*)(intptr_t)t);
	}
	adder(0);

	int failed = 0;
	for(int t = 1; t < THREADS; ++t) {
		intptr_t f;
	    pthread_join(th[t], (void*)&f);
		failed += f;
	}

	return verify(failed);
}

int verify(int failed) {
	Node* n = root;
	int count = 0;
	
#ifdef RESULTCHECK
	intptr_t out[DATALENGTH];
#endif
	
	while(n && count < DATALENGTH+1) {
#ifdef RESULTCHECK
		out[count] = n->data;
#endif
		count++;
		n = n->next;
	}
#ifdef RESULTCHECK
	printf(
		"RESULTCHECK"
		" %u %u %u %u %u %u"
#if DATALENGTH > 6
		" %u"
#endif		
#if DATALENGTH > 7
		" %u"
#endif		
#if DATALENGTH > 8
		" %u"
#endif		
#if DATALENGTH > 9
		" %u"
#endif
		"\n"
	, out[0], out[1], out[2], out[3], out[4], out[5]
#if DATALENGTH > 6
	,	out[6]
#endif		
#if DATALENGTH > 7
	,	out[7]
#endif		
#if DATALENGTH > 8
	,	out[8]
#endif		
#if DATALENGTH > 9
	,	out[9]
#endif
	);
#endif
	assert(count + failed == DATALENGTH);
    return 0;
}
