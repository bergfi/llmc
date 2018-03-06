#include <assert.h>

int main(int argc, char** argv) {
	int i = 5;
	int j = 6;
	i += j;
	assert(i == 11);
	return i;
}

