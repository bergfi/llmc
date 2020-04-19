#include <assert.h>

int g = 1234;

int main(int argc, char** argv) {
	int d = 6;
	__atomic_load(&g, &d, 0);
	assert(g==1234);
	return 0;
}

