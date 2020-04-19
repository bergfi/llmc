#include <assert.h>

__int128_t g = 1234;

int main(int argc, char** argv) {
	__int128_t e = 1234;
	__int128_t d = 6;
	int b = __atomic_compare_exchange(&g, &e, &d, 0, 0, 0);
	assert(b);
	assert(g==6);
	assert(e==1234);
	assert(d==6);
	return 0;
}

