#include <assert.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>

// in ints
#define CHUNK_SIZE 16

// max per var
#define MAX 10

// number of vars
#define VARS 6

// number of chunk types
#define CHUNKTYPES 6

int nextLong(model_t model, int group, int* src, TransitionCB cb, void* ctx) {
	chunk chOld = pins_chunk_get(model, group % CHUNKTYPES, src[group]);
	int v = ((int*)chOld.data)[0];
	if(v >= MAX -1) return 0;

	int dst[VARS];

	int ch[CHUNK_SIZE];
	for(int i=0; i < CHUNK_SIZE; ++i) {
		ch[i] = v + 1;
	}
	memmove(dst, src, VARS*sizeof(int));

	chunk chNew;
	chNew.len = CHUNK_SIZE*sizeof(int);
	chNew.data = (char*)ch;

	dst[group] = pins_chunk_put(model, group % CHUNKTYPES, chNew);
	transition_info_t ti = {0};
	ti.group = group;
	cb(ctx, &ti, dst, NULL);
	return 1;
}

int nextAll(model_t model, int* src, TransitionCB cb, void* ctx) {
	int n = 0;
	for(int i=0; i < VARS; ++i) {
		n += nextLong(model, i, src, cb, ctx);
	}
	return n;
}

int pins_model_init(model_t model) {
	matrix_t* dm = (matrix_t*)malloc(sizeof(matrix_t));
	dm_create(dm, VARS, VARS);
	dm_fill(dm);

	lts_type_t lts = lts_type_create();

	lts_type_set_state_length(lts, VARS);

	for(int i = 0; i < CHUNKTYPES; ++i) {
		int n;
		char name[5];
		name[0] = 0;
		strcpy(name, "typ");
		name[3] = '0' + i;
		name[4] = '\0';
		int ti = lts_type_add_type(lts, name, &n);
        assert(ti == i);
		printf("added type %i\n", ti);
	}

	for(int i = 0; i < VARS; ++i) {
		char name[5];
        name[0] = 0;
		strcmp(name, "var");
		name[3] = '0' + i;
		name[4] = '\0';
		lts_type_set_state_name(lts, i, name);
		//lts_type_set_state_type(lts, i, name);
		lts_type_set_state_typeno(lts, i, i % CHUNKTYPES);
	}


	GBsetLTStype(model, lts);

	GBsetDMInfo(model, dm);
	GBsetNextStateLong(model, nextLong);
	GBsetNextStateAll(model, nextAll);

	int init[VARS] = {0};

	for(int i = 0; i < VARS; ++i) {
		int n;

		int ch[CHUNK_SIZE];
		for(int j=0; j < CHUNK_SIZE; ++j) {
			ch[j] = 0;
		}
		chunk c;
		c.len = CHUNK_SIZE*sizeof(int);
		c.data = (char*)ch;
		init[i] = pins_chunk_put(model, i % CHUNKTYPES, c);

	}

	GBsetInitialState(model, init);

	return 0;

}

const char pins_plugin_name[] ="test";
