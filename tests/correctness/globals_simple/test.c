#include <assert.h>

int g = 5;

void lol() {
	g = 7;
}

int main(int argc, char** argv) {
	int i = 5;
	g++;
	lol();
	i += g;
	g += i;
	assert(g==19);
	return i;
}

