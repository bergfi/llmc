#include <assert.h>
#include <stdint.h>

typedef struct {
	uint64_t a;
	uint64_t b;
} S;

S g = {1,2};

volatile S* gp;

int main(int argc, char** argv) {
	gp = &g;
	S e = {1, 2};
	S d = {3, 4};
	S d2 = {5, 6};
	int b = __atomic_compare_exchange(gp, &e, &d, 0, 0, 0);
	assert(b);
	assert(g.a==3);
	assert(g.b==4);
	assert(e.a==1);
	assert(e.b==2);
	assert(d.a==3);
	assert(d.b==4);
	int b2 = __atomic_compare_exchange(gp, &d, &d2, 0, 0, 0);
	assert(b);
	assert(g.a==5);
	assert(g.b==6);
	assert(e.a==1);
	assert(e.b==2);
	assert(d.a==3);
	assert(d.b==4);
	assert(d2.a==5);
	assert(d2.b==6);
	return 0;
}

