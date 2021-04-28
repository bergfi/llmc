#include <stddef.h>
#include <dmc/dmcapi.h>

static void thisfunctionexiststomakesuretheapiisinthellvmir(void) {
    dmc_init(NULL);
    dmc_init_cb(NULL);
    dmc_nextstates(NULL, 0);
    dmc_initialstate(NULL);

    dmc_insert    (NULL, NULL, 0, 0);
    dmc_insertB   (NULL, NULL, 0, 0);
    dmc_transition(NULL, 0, NULL, 0, 0);
    dmc_delta     (NULL, 0, 0, NULL, 0, 0);
    dmc_deltaB    (NULL, 0, 0, NULL, 0, 0);
    dmc_get       (NULL, 0, NULL, 0);
    dmc_getpart   (NULL, 0, 0, NULL, 0, 0);
    dmc_getpartB  (NULL, 0, 0, NULL, 0, 0);

    dmc_big_nextstates(NULL, 0);
    dmc_big_initialstate(NULL);

    dmc_big_insert (NULL, NULL, 0, 0);
    dmc_big_delta  (NULL, 0, 0, NULL, 0, 0);
    dmc_big_get    (NULL, 0, NULL, 0);
    dmc_big_getpart(NULL, 0, 0, NULL, 0, 0);
}
