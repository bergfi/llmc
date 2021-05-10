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

#pragma once

#include <dmc/model.h>

class LLVMModel: public VModel<llmc::storage::StorageInterface> {
public:
//    size_t getNextAll(StateID const& s, Context* ctx) override {
//        auto state = ctx->getModelChecker()->getState(ctx, s);
//        assert(state);
//        StateSlot d[state->getLength()];
//        size_t imax = state->getLength();
//        for(size_t i = 0; i < imax; i += 2) {
//            memcpy(d, state->getData(), sizeof(StateSlot)*state->getLength());
//            d[i] = (state->getData()[i] + 1) % 10;
//            ctx->getModelChecker()->newTransition(ctx, state->getLength(), d, TransitionInfoUnExpanded::None());
//        }
//        return state->getLength();
//    }
    size_t getNextAll(StateID const& s, Context* ctx) override {
        auto state = ctx->getModelChecker()->getState(ctx, s);
        assert(state);
//        fprintf(stderr, "LLVMModel::getNextAll %u\n", state->getLength());
//        StateSlot d[state->getLength()];

        size_t buffer[3];

        size_t imax = state->getLength();
        for(size_t i = 0; i < imax; i += 2) {
            StateSlot diff[] = {(state->getData()[i] + 1) % 10, 0};
            ctx->getModelChecker()->newTransition(ctx, llmc::storage::StorageInterface::Delta::create((llmc::storage::StorageInterface::Delta*)buffer, i, diff, 2), TransitionInfoUnExpanded::None());
        }
        return state->getLength();
    }

    size_t getInitial(Context* ctx) override {
        StateSlot d[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        ctx->getModelChecker()->newState(ctx, 0, sizeof(d)/sizeof(*d), d).getState();
        return 1;
    }

    llmc::statespace::Type* getStateVectorType() override {
        return nullptr;
    }
};