#include <assert.h>

int* g;

void lol() {
	(*g)++;
}

int main(int argc, char** argv) {
	int i = 5;
	g = &i;
	lol();
	assert(i==6);
	assert(*g==6);
	return 0;
}

