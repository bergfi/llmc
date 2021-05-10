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

#include <llmc/ssgen.h>

model* model::getPINS(std::string const& filename) {
    return model_pins_so::get(filename);
}

int model_pins_so::getNextAll(TypeInstance const& s, ssgen* ctx) {
    //_pins_getnextall(nullptr, (int*)s.data(), (TransitionCB)_reportTransition_cb, ctx);
    auto cb = (TransitionCB)ctx->getCB_ReportTransition();
    for(int tg = 0; tg < this->_tgroups; ++tg) {
        _pins_getnext((model_t)ctx, tg, ((int*)s.data()), cb, ctx);
    }
    return 0;
}

int model_pins_so::getNextAll(size_t len, void const* data, ssgen* ctx) {
    //_pins_getnextall(nullptr, (int*)s.data(), (TransitionCB)_reportTransition_cb, ctx);
    auto cb = (TransitionCB)ctx->getCB_ReportTransition();
    for(int tg = 0; tg < this->_tgroups; ++tg) {
        _pins_getnext((model_t)ctx, tg, ((int*)data), cb, ctx);
    }
    return 0;
}

int doPrint = 0;
