#include <assert.h>

int g = 5;

void lol(int v, int i) {
	int j = 0;
	i++;
	j++;
	g = v + i;
}

int main(int argc, char** argv) {
	int i = 5;
	lol(10, i);
	i += g;
	assert(i==21);
	return i;
}

