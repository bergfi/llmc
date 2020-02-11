#pragma once

#include <ostream>
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
public:

    using TransitionInfo = typename Model::TransitionInfo;
    using StateID = typename ModelChecker::StateID;
    using StateSlot = typename ModelChecker::StateSlot;

    DotPrinter(std::ostream& out): out(out) {

    }

    void init() {
        out << "digraph somegraph {" << std::endl;
    }

    void finish() {
        out << "}" << std::endl;
    }

    void writeState(Model* model, StateID const& stateID, size_t length, const StateSlot* slots) {
        out << "\ts" << stateID.getData();

//        out << " [shape=record, label=\"{" << stateID.getData();
//        out << "|{{state|{";
//        writeVector(out, model, length, slots);
//        out << "}}}";
//        out << "}\"]";

        out << ";" << std::endl;
    }

    void writeTransition(StateID const& from, StateID const& to, TransitionInfo const& tInfo) {
        TransitionInfo const& t = tInfo;
        out << "\ts" << from.getData() << " -> s" << to.getData();
        if(t.label.length() > 0) {
            out << " [label=\"" << t.label << "\"]";
        } else {
            out << " []";
        }
        out << ";" << std::endl;
    }

    void writeVector(ostream& out, Model* model, size_t length, const StateSlot* slots) {
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

private:
    std::ostream& out;
};

} // namespace statespace

} // namespace llmc
