#include <assert.h>

int** g;

void lol() {
	(**g)++;
}

int main(int argc, char** argv) {
	int i = 5;
	int* p = &i;
	g = &p;
	lol();
	assert(i==6);
	assert(*p==6);
	assert(**g==6);
	return 0;
}

