#pragma once

#include <llmc/model.h>

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
        StateSlot d[state->getLength()];

        size_t buffer[3];

        size_t imax = state->getLength();
        for(size_t i = 0; i < imax; i += 2) {
            StateSlot diff[] = {(state->getData()[i] + 1) % 10, 0};
            ctx->getModelChecker()->newTransition(ctx, llmc::storage::StorageInterface::Delta::create((llmc::storage::StorageInterface::Delta*)buffer, i, diff, 2), TransitionInfoUnExpanded::None());
        }
        return state->getLength();
    }

    StateID getInitial(Context* ctx) override {
        StateSlot d[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        return ctx->getModelChecker()->newState(0, sizeof(d)/sizeof(*d), d).getState();
    }

    llmc::statespace::Type* getStateVectorType() override {
        return nullptr;
    }
};