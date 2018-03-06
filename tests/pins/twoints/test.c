#include <ltsmin/pins.h>

#define MAX 1000
#define VARS 2

int nextLong(model_t model, int group, int* src, TransitionCB cb, void* ctx) {
	if(src[group] >= MAX -1) return 0;
	int dst[VARS];
	memmove(dst, src, VARS*sizeof(int));
	dst[group]++;
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
	matrix_t dm;
	dm_create(&dm, VARS, VARS);
	dm_fill(&dm);

	lts_type_t lts = lts_type_create();

	lts_type_set_state_length(lts, VARS);

	for(int i = 0; i < VARS; ++i) {
		int n;
		char name[5];
		name[0] = 0;
		strcmp(name, "typ");
		name[3] = '0' + i;
		name[4] = '\0';
		int ti = lts_type_add_type(lts, name, &n);
		name[0] = 0;
		strcmp(name, "var");
		name[3] = '0' + i;
		name[4] = '\0';
		lts_type_set_state_name(lts, i, name);
		//lts_type_set_state_type(lts, i, name);
		lts_type_set_state_typeno(lts, i, ti);
	}

	int init[VARS] = {0};

	GBsetLTStype(model, lts);

	GBsetDMInfo(model, &dm);
	GBsetInitialState(model, init);
	GBsetNextStateLong(model, nextLong);
	GBsetNextStateAll(model, nextAll);

	return 0;

}

const char pins_plugin_name[] ="test";
