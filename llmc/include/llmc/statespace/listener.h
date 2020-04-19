#pragma once

#include <atomic>
#include <ostream>
#include <shared_mutex>
#include <libfrugi/Settings.h>

using namespace libfrugi;

#include <llmc/models/PINSModel.h>

namespace llmc {

namespace statespace {

template<typename ModelChecker, typename Model>
class Listener {
public:

    using StateID = typename ModelChecker::StateID;

    Listener(std::ostream& out) {}

//    void init();
//
//    void finish();
//
//    void writeState(StateID const& stateID);
//
//    template<typename TransitionInfoGetter>
//    void writeTransition(StateID const& from, StateID const& to);
};

template<typename ModelChecker, typename Model>
class VoidPrinter {
public:

    using StateID = typename ModelChecker::StateID;

    VoidPrinter() {}

    VoidPrinter(std::ostream& out) {}

    void init() {}

    void finish() {}

    void writeState(StateID const& stateID) {}

    void writeTransition(StateID const& from, StateID const& to) {}
};

template<typename ModelChecker, typename Model>
class DotPrinter {
public:
    using Context = typename ModelChecker::Context;
    using mutex_type = std::mutex;
    using updatable_lock = std::unique_lock<mutex_type>;
public:

    using TransitionInfo = typename Model::TransitionInfo;
    using StateID = typename ModelChecker::StateID;
    using FullState = typename ModelChecker::FullState;
    using StateSlot = typename ModelChecker::StateSlot;

    DotPrinter(): out(std::cerr), _writeState(false) {

    }
    DotPrinter(std::ostream& out): out(out), _writeState(false) {

    }

    void init() {
        updatable_lock lock(mtx);
        out << "digraph somegraph {" << std::endl;
    }

    void finish() {
        updatable_lock lock(mtx);
        out << "}" << std::endl;
    }

    void writeState(Model* model, StateID const& stateID, const FullState* fsd) {
        if(!_writeSubState && fsd && !fsd->isRoot()) {
            return;
        }
        updatable_lock lock(mtx);
        out << "\ts" << stateID.getData();

        out << " [shape=record, label=\"{" << (stateID.getData() & 0xFFFFFFFFFFULL) << " (" << (stateID.getData() >> 40) << ")";
        out << "|{{state|{";
        if(_writeState && fsd) writeVector(out, model, fsd->getLength(), fsd->getData());
        out << "}}}";
        out << "}\"]";

        out << ";" << std::endl;
    }

    void writeTransition(StateID const& from, StateID const& to, TransitionInfo tInfo) {
        updatable_lock lock(mtx);
        TransitionInfo const& t = tInfo;

        size_t start_pos = 0;
        std::string escape = "\"";
        std::string escaped = "\\\"";
        std::string str = tInfo.label;
        while((start_pos = str.find(escape, start_pos)) != std::string::npos) {
            str.replace(start_pos, escape.length(), escaped);
            start_pos += escaped.length();
        }

        out << "\ts" << from.getData() << " -> s" << to.getData();
        if(str.length() > 0) {
            out << " [label=\"" << str << "\"]";
        } else {
            out << " []";
        }
        out << ";" << std::endl;
    }

    void writeVector(std::ostream& out, Model* model, size_t length, const StateSlot* slots) {
        for(size_t i = 0; i < length; ++i) {
            if(i > 0) {
                out << "|";
            }
            out << "{" << i << "|" << slots[i] << "}";
        }
//        for(int i=0, first=1; i<lts_type_get_state_length(lts_file_get_type(file)); ++i) {
//            if(first) { first = 0; } else { fprintf(file->file, "|"); }
//            const char* name = lts_type_get_state_name(lts_file_get_type(file), i);
//            fprintf(file->file, "{%.3s|%u}", name?name:"??????" , state[i]);
//            //int typeNo = lts_type_get_state_typeno(lts_file_get_type(file), i);
//            //printChunkType(file, lts_type_get_state_name(lts_file_get_type(file), i), typeNo, state[i]);
//        }
    }

    void setSettings(Settings& settings) {
        _writeState = settings["listener.writestate"].isOn();
        _writeSubState = settings["listener.writesubstate"].isOn();
    }

private:
    std::ostream& out;
    mutex_type mtx;
    bool _writeState;
    bool _writeSubState;
};

template<typename ModelChecker, typename Model>
class StatsGatherer {
public:
    using Context = typename ModelChecker::Context;
public:

    using TransitionInfo = typename Model::TransitionInfo;
    using StateID = typename ModelChecker::StateID;
    using StateSlot = typename ModelChecker::StateSlot;
    using FullState = typename ModelChecker::FullState;

    StatsGatherer(): _states(0), _transitions(0) {
    }

    void init() {
    }

    void finish() {
    }

    void writeState(Model* model, StateID const& stateID, const FullState* fsd) {
        ++_states;
    }

    void writeTransition(StateID const& from, StateID const& to, TransitionInfo const& tInfo) {
        ++_transitions;
    }

    void writeVector(std::ostream& out, Model* model, size_t length, const StateSlot* slots) {
    }

    size_t getStates() const {
        return _states;
    }

    size_t getTransitions() const {
        return _transitions;
    }

    void setSettings(Settings& settings) {
    }

private:
    std::atomic<size_t> _states;
    std::atomic<size_t> _transitions;
};

} // namespace statespace

} // namespace llmc
