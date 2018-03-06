#include <assert.h>

int main(int argc, char** argv) {
	int i = 5;
	i++;
	assert(i == 6);
	return i;
}

