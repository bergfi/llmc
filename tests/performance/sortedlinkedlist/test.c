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
	Node* t = __atomic_load_n(target, __ATOMIC_SEQ_CST);
	do {
		while(t) {
			if(__atomic_load_n(&t->data, __ATOMIC_SEQ_CST) >= data) break;
			target = &t->next;
			t = __atomic_load_n(target, __ATOMIC_SEQ_CST);
		}
		node->next = t;
	} while(!__atomic_compare_exchange_n(target, &t, node, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
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
	Node* p = NULL;
	Node* n = root;
	int count = 0;
	int ok = 1;
	while(n && count < DATALENGTH+1) {
		if(p) {
			if(p->data >= n->data) {
				ok = 0;
			}
		}
		p = n;
		count++;
		n = n->next;
	}
	assert(count + failed == DATALENGTH);
	assert(ok);
	return !ok;
}
