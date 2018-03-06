#include <assert.h>

volatile int g = 0;
volatile int gread;

int main(int argc, char** argv) {
	int register i = 4;
	int total = 1;
	gread = 1;
	while(i) {
		total *= i;
		i -= gread;
	}
	g = total;
	assert(g!=24);
	return i;
}

