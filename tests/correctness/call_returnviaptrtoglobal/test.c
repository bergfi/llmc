#include <assert.h>

int g;

void lol(int* gp) {
	int i = 1;
	int j = 2;
	int k = 3;
	int l = 4;
	*gp += i * j * k * l;
}

int main(int argc, char** argv) {
	int i = 5;
	lol(&g);
	i += 2 + g;
	assert(i==31);
	return i;
}

