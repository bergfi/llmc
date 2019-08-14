#pragma once

#include <cassert>
#include <string>

#include <dlfcn.h>
extern "C" {
#include <ltsmin/pins.h>
}

template<typename MODELCHECKER>
class Model {
public:

    using ModelChecker = MODELCHECKER;

    virtual size_t getNextAll(typename ModelChecker::StateID const& s, typename ModelChecker::Context& ctx) = 0;
    virtual typename ModelChecker::StateID getInitial(typename ModelChecker::Context& ctx) = 0;
};
