#include <assert.h>

int lol() {
	int i = 1;
	int j = 2;
	int k = 3;
	int l = 4;
	return i * j * k * l;
}

int main(int argc, char** argv) {
	int i = 5;
	i += lol();
	i += 2;
	assert(i==31);
	return i;
}

