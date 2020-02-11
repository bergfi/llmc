#pragma once

template<typename STORAGE>
class VContext;
template<typename STORAGE>
class VModel;

#include <cassert>
#include <string>
#include <dlfcn.h>

extern "C" {
#include <ltsmin/pins.h>
}

#include <llmc/statespace/statetype.h>
#include <llmc/storage/interface.h>
#include <llmc/modelcheckers/interface.h>
#include <llmc/transitioninfo.h>

class Model {
public:

//    size_t getNextAll(typename MODELCHECKERINTERFACE::StateID const& s, typename MODELCHECKERINTERFACE::Context& ctx);
//    typename ModelChecker::StateID getInitial(typename MODELCHECKERINTERFACE::Context& ctx);
//    llmc::statespace::Type* getStateVectorType();

    struct TransitionInfo {
        std::string label;

        TransitionInfo(): label("") {}
        TransitionInfo(const std::string &label) : label(label) {}
    };
};

template<typename STORAGE>
class VContext {
public:
    using StateID = llmc::storage::StorageInterface::StateID;
    using StateSlot = llmc::storage::StorageInterface::StateSlot;

public:
    VModelChecker<STORAGE>* getModelChecker() {
        return mc;
    };
    StateID getSourceState() const {
        return sourceState;
    }

    VContext(VModelChecker<STORAGE>* mc_, VModel<STORAGE>* model): model(model), mc(mc_), sourceState(0) {}

    template<typename T>
    T* getModel() {
        return (T*)model;
    }

    VModel<STORAGE>* getModel() {
        return model;
    }

public:
    VModel<STORAGE>* model;
    VModelChecker<STORAGE>* mc;
    StateID sourceState;
};

template<typename STORAGE>
class VModel: public Model {
public:

    struct TransitionInfo {
        std::string label;

        TransitionInfo(): label("") {}
        TransitionInfo(const std::string &label) : label(label) {}
    };

    using StateID = typename STORAGE::StateID;
    using StateSlot = typename STORAGE::StateSlot;
//    using Context = ContextBase<VModel<MODELCHECKERINTERFACE>>;
    using Context = VContext<STORAGE>;

    virtual void init(Context* ctx) = 0;

    virtual size_t getNextAll(StateID const& s, Context* ctx) = 0;
    virtual StateID getInitial(Context* ctx) = 0;
    virtual llmc::statespace::Type* getStateVectorType() = 0;
    virtual TransitionInfo getTransitionInfo(VContext<llmc::storage::StorageInterface>* ctx, TransitionInfoUnExpanded const& tinfo_) const = 0;
    virtual size_t getStateLength() const = 0;
};