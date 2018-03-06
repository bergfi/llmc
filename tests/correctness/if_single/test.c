#include <assert.h>

int main(int argc, char** argv) {
	int i = 0;
	if(argc) {
		i = 1;
	} else {
		i = 2;
	}
	assert(i==2); // argc is 0 for now
	return i;
}

