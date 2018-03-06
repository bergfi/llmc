#include <assert.h>

void lol() {
	int i = 0,j = 0;
	i++;
	j++;
}

int main(int argc, char** argv) {
	int i = 5;
	lol();
	i++;
	assert(i==6);
	return i;
}

