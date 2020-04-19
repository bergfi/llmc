#include <assert.h>

int a = 0;
int* ap = &a;

int eq(int* p1, int* p2, int* p3) {
	return p1 == p2 && p2 == p3;
}

int main(int argc, char** argv) {
	int *c = &argc;
	int *d = &a;
	assert(!eq(&a, ap, c));
	assert(!eq(&a, ap, d));
}

