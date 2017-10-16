#include <assert.h>

int g = 5;

void lol(int v) {
	int i = 0,j = 0;
	i++;
	j++;
	g = v;
}

int main(int argc, char** argv) {
	int i = 5;
	lol(10);
	i += g;
	assert(i==15);
	return i;
}

