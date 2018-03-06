#include <assert.h>

int g = 5;

void lol() {
	int i,j;
	i++;
	j++;
	g = 10;
}

int main(int argc, char** argv) {
	int i = 5;
	i+=g;
	lol();
	i+=g;
	assert(i==21);
	return i;
}

