#include <assert.h>

typedef struct {
	int * var;
	int somedata;
} S;

volatile S* gs;

int main(int argc, char** argv) {
	S ls;
	gs = &ls;

	int l = 1234;
	ls.var = &l;

	int e = 1234;
	int d = 6;
	int b = __atomic_compare_exchange(gs->var, &e, &d, 0, 0, 0);
	assert(b);
	assert(l==6);
	assert(*ls.var==6);
	assert(e==1234);
	assert(d==6);
	return 0;
}

