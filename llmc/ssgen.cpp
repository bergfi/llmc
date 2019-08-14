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
