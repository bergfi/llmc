#include <assert.h>

int g = 5;

void lol(int* gp) {
	(*gp)++;
}

int main(int argc, char** argv) {
//	int i = 5;
//	g++;
	lol(&g);
//	i += g;
//	g += i;
	assert(g==6);
	return 0;
}

