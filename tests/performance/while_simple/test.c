#include <assert.h>

volatile int g = 0;
volatile int gread;

int main(int argc, char** argv) {
	int register i = 1<<17;
	int register total = 1;
	gread = 1;
	while(i) {
		g *= i;
		i -= gread;
	}
	//g = total;
	assert(g==23);
	return i;
}

