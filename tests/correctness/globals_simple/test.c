#include <assert.h>

int g = 5;

void lol() {
	g++;
}

int main(int argc, char** argv) {
//	int i = 5;
	g++;
//	lol();
//	i += g;
//	g += i;
	assert(g==6);
	return 0;
}

