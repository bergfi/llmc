#include <assert.h>

int g = 1234;

int main(int argc, char** argv) {
	int e = 1234;
	int d = 6;
	int b = __atomic_compare_exchange(&g, &e, &d, 0, 0, 0);
	assert(b);
	assert(g==6);
	assert(e==1234);
	assert(d==6);
	return 0;
}

