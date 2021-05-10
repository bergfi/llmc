/*
 * LLMC - LLVM IR Model Checker
 * Copyright © 2013-2021 Freark van der Berg
 *
 * This file is part of LLMC.
 *
 * LLMC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * LLMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LLMC.  If not, see <https://www.gnu.org/licenses/>.
 */

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
