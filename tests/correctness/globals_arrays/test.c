#include <assert.h>

int g[4];

void lol(int i) {
	g[i]++;
}

int main(int argc, char** argv) {
	assert(g[0] == 0);
	assert(g[1] == 0);
	assert(g[2] == 0);
	assert(g[3] == 0);
	lol(1);
	lol(2);
	lol(3);
	lol(1);
	lol(2);
	lol(2);
	lol(2);
	assert(g[0] == 0);
	assert(g[1] == 2);
	assert(g[2] == 4);
	assert(g[3] == 1);
	return 0;
}

