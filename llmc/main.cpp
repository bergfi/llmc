#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <libfrugi/MessageFormatter.h>
#include <libfrugi/Shell.h>
#include <libfrugi/System.h>
#include <llmc/ll2pins.h>
#include <sstream>

#define VERBOSITY_SEARCHING 2

using namespace llmc;

bool findBinary(std::string const& name, MessageFormatter& messageFormatter, File& binary) {
    std::vector<File> results;
    bool accessible = false;
    FileSystem::findInPath(results, File(name));
    for(File binFound: results) {
        accessible = false;
        if(FileSystem::hasAccessTo(binFound, X_OK)) {
            accessible = true;
            binary = binFound;
            break;
        } else {
            messageFormatter.reportWarning(name + " [" + binFound.getFilePath() + "] is not runnable", VERBOSITY_SEARCHING);
        }
    }
    if(!accessible) {
        messageFormatter.reportError("no runnable " + name + " executable found in PATH");
    } else {
        messageFormatter.reportAction("Using " + name + " [" + binary.getFilePath() + "]", VERBOSITY_SEARCHING);
    }

    return !accessible;
}

bool compile(File const& bin_llc, File const& input, File const& output, MessageFormatter& out) {
    Shell::RunStatistics stats;
    Shell::SystemOptions sysOps;

    out.notify("Compiling...");

    // llc -filetype=obj helloworld_pins.ll -relocation-model=pic
    sysOps.command = bin_llc.getFilePath()
                   + " -filetype=obj"
                   + " -relocation-model=pic"
                   + " " + input.getFilePath()
                   + " -o " + output.getFilePath()
                   ;
    sysOps.cwd = input.getPathTo();
    sysOps.verbosity = 1;
    Shell::system(sysOps, &stats);
    std::stringstream ss;
    ss << "Took " << stats.time_monraw << " second (user: " << stats.time_user << ", system: " << stats.time_system << ")";
    out.reportAction(ss.str());
}

bool link(File const& bin_cc, File const& input, File const& output, MessageFormatter& out) {
    Shell::RunStatistics stats;
    Shell::SystemOptions sysOps;

    out.notify("Linking...");

    std::string llmcosobjectname = "libllmcos.a";

    std::vector<File> tries;
    tries.emplace_back(File(System::getBinaryLocation(), llmcosobjectname));
    tries.emplace_back(File(System::getBinaryLocation() + "/../libllmcos", llmcosobjectname));
    tries.emplace_back(File(std::string(CompileOptions::CMAKE_INSTALL_PREFIX) + "/share/llmc", llmcosobjectname));

    File libLLMCOSObject;
    for(auto& f: tries) {
        if(f.exists()) {
            libLLMCOSObject = f;
            break;
        }
    }

    if(libLLMCOSObject.isEmpty()) {
        out.reportError("Could not find LLMC OS, tried:");
        for(auto& f: tries) {
            out.reportAction2(f.getFilePath());
        }
    } else {
        out.reportAction("Using LLMC OS [" + libLLMCOSObject.getFilePath() + "]");
    }

    // gcc -g -shared helloworld_pins.o ../../../libllmcos.o -o a.so
    sysOps.command = bin_cc.getFilePath()
                   + " -g -shared"
                   + " " + input.getFilePath()
                   + " " + libLLMCOSObject.getFilePath()
                   + " -o " + output.getFilePath()
                   ;
    sysOps.cwd = input.getPathTo();
    sysOps.verbosity = 1;
    Shell::system(sysOps, &stats);
    std::stringstream ss;
    ss << "Took " << stats.time_monraw << " second (user: " << stats.time_user << ", system: " << stats.time_system << ")";
    out.reportAction(ss.str());
}

//bool model_check(File const& bin_ltsmin, File const& input, File const& output, MessageFormatter& out) {
//}
//
//bool dot(File const& bin_dot, File const& input, File const& output, MessageFormatter& out) {
//}

int main(int argc, char* argv[]) {

    MessageFormatter out(std::cout);
    out.setAutoFlush(true);
    out.useColoredMessages(true);
    out.setVerbosity(5);

    Shell::messageFormatter = &out;

    // Find binaries
    File bin_llc;
    File bin_cc;
    File bin_ltsmin;
    File bin_dot;
    findBinary("llc", out, bin_llc);
    if(findBinary("gcc", out, bin_cc)) {
        findBinary("clang", out, bin_cc);
    }
    findBinary("pins2lts-seq", out, bin_ltsmin);
    findBinary("dot", out, bin_dot);

    // init libfrugi
    System::init(argc, argv);

    if(argc<2) {
        out.reportError("No input file specified");
        exit(-1);
    }

    File input(System::getArgument(1));
    input.fix();
    File output_ll = input.newWithExtension("pins.ll");
    File output_o = input.newWithExtension("pins.o");
    File output_so = input.newWithExtension("so");
    File output_dot = input.newWithExtension("dot");
    File output_png = input.newWithExtension("png");

    if(!FileSystem::hasAccessTo(input, R_OK)) {
        out.indent();
        out.reportError("Cannot read input file (" + input.getFilePath() + ")");
        out.outdent();
        exit(1);
    }
    if(FileSystem::isDir(input)) {
        out.reportError("Input file (" + input.getFilePath() + ") is a directory");
        exit(1);
    }

    for(auto& f: {output_ll, output_o, output_so}) {
        if(FileSystem::isDir(f)) {
            out.reportError("Output file (" + f.getFilePath() + ") is a directory");
            exit(1);
        }
        if(!FileSystem::canCreateOrModify(f)) {
            out.reportError("Cannot create or modify output file (" + f.getFilePath() + ")");
            exit(1);
        }
    }

    // .ll -> .pins.ll
    if(ll2pins(input, output_ll)) {
        out.reportError("Pinsifying failed");
        exit(1);
    }

    // .pins.ll -> .o
    if(compile(bin_llc, output_ll, output_o, out)) {
        out.reportError("Compiling failed");
        exit(1);
    }

    // .o -> .so
    if(link(bin_cc, output_o, output_so, out)) {
        out.reportError("Linking failed");
        exit(1);
    }

    // LTSmin
//    if(model_check(bin_ltsmin, output_so, output_dot, out)) {
//        out.reportError("Model checking failed");
//        exit(1);
//    }

    // Dot
//    if(dot(bin_dot, output_dot, output_png, out)) {
//        out.reportError("Printing to PNG failed");
//        exit(1);
//    }

    return 0;
}
