#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <libfrugi/MessageFormatter.h>
#include <libfrugi/Settings.h>
#include <libfrugi/Shell.h>
#include <libfrugi/System.h>
#include <llmc/ll2dmc.h>
//#include <llmc/ssgen.h>
#include <dmc/modelcheckers/interface.h>
#include <dmc/modelcheckers/multicoresimple.h>
#include <dmc/modelcheckers/singlecore.h>
#include <dmc/modelcheckers/multicore_bitbetter.h>
#include <dmc/statespace/listener.h>
#include <dmc/storage/interface.h>
#include <dmc/storage/dtree.h>
#include <dmc/storage/dtree2.h>
#include <dmc/storage/stdmap.h>
#include <dmc/storage/cchm.h>
#include <dmc/storage/treedbs.h>
#include <dmc/storage/treedbsmod.h>
#include <sstream>
#include <dmc/common/murmurhash.h>

#include <dmc/models/DMCModel.h>
#include <libllmc/LLVMModel.h>

#define VERBOSITY_SEARCHING 2

using namespace llmc;
using namespace libfrugi;

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
                   + " -O=3"
                   + " " + input.getFilePath()
                   + " -o " + output.getFilePath()
                   ;
    sysOps.cwd = input.getPathTo();
    sysOps.verbosity = 1;
    Shell::system(sysOps, &stats);
    std::stringstream ss;
    ss << "Took " << stats.time_monraw << " second (user: " << stats.time_user << ", system: " << stats.time_system << ")";
    out.reportAction(ss.str());
    return !output.exists();
}

bool link(File const& bin_cc, File const& input, File const& output, MessageFormatter& out) {
    Shell::RunStatistics stats;
    Shell::SystemOptions sysOps;

    out.notify("Linking...");

    std::string llmcvmobjectname = "libllmcvm.a";

    std::vector<File> tries;
    tries.emplace_back(System::getBinaryLocation(), llmcvmobjectname);
    tries.emplace_back(System::getBinaryLocation() + "/../libllmcos", llmcvmobjectname);
    tries.emplace_back(std::string(CompileOptions::CMAKE_INSTALL_PREFIX) + "/share/llmc", llmcvmobjectname);

    File libLLMCVMObject;
    for(auto& f: tries) {
        if(f.exists()) {
            libLLMCVMObject = f;
            break;
        }
    }

    if(libLLMCVMObject.isEmpty()) {
        out.reportError("Could not find LLMC VM, tried:");
        for(auto& f: tries) {
            out.reportAction2(f.getFilePath());
        }
    } else {
        out.reportAction("Using LLMC OS [" + libLLMCVMObject.getFilePath() + "]");
    }

    // gcc -g -shared helloworld_pins.o ../../../libllmcvm.o -o a.so
    sysOps.command = bin_cc.getFilePath()
                   + " -g -shared"
                   + " " + input.getFilePath()
                   + " -O3"
                   + " " + libLLMCVMObject.getFilePath()
                   + " -o " + output.getFilePath()
                   ;
    sysOps.cwd = input.getPathTo();
    sysOps.verbosity = 1;
    Shell::system(sysOps, &stats);
    std::stringstream ss;
    ss << "Took " << stats.time_monraw << " second (user: " << stats.time_user << ", system: " << stats.time_system << ")";
    out.reportAction(ss.str());
    return !output.exists();
}

//bool model_check(File const& bin_ltsmin, File const& input, File const& output, MessageFormatter& out) {
//}
//
//bool dot(File const& bin_dot, File const& input, File const& output, MessageFormatter& out) {
//}

template<typename T>
struct HashCompareMurmur {
    static constexpr uint64_t hashForZero = 0x7208f7fa198a2d81ULL;
    static constexpr uint64_t seedForZero = 0xc6a4a7935bd1e995ULL*8ull;

    __attribute__((always_inline))
    bool equal( const T& j, const T& k ) const {
        return j == k;
    }

    __attribute__((always_inline))
    size_t hash( const T& k ) const {
        return MurmurHash64(&k, sizeof(T), seedForZero);
    }
};


template<typename Storage, template<typename,typename> typename Printer = llmc::statespace::VoidPrinter, template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goDMC(MessageFormatter& out, std::string soFile) {
    Settings& settings = Settings::global();

    ofstream f;
    f.open("out.dot", std::fstream::trunc);

    using MC = ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, Printer>;

//    PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>* model = PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>::get(soFile);
//    llmc::statespace::DotPrinter< ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>
//                                , PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>
//                                > printer(f);
//    ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter> mc(model, printer);

    VModel<llmc::storage::StorageInterface>* model = DMCModel::get(soFile);
    if(model) {
        Printer<MC, VModel<llmc::storage::StorageInterface>> printer(f);
        printer.init();
        MC mc(model, printer);

        //    VModel<llmc::storage::StorageInterface>* model = new LLVMModel();
        //    llmc::statespace::DotPrinter< ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>
        //                                , VModel<llmc::storage::StorageInterface>
        //                                > printer(f);
        //    ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter> mc(model, printer);

        //    if constexpr(Storage::stateHasFixedLength()) {
        //        if(settings["storage.fixedvectorsize"].asUnsignedValue() > 0) {
        //            mc.getStorage().setMaxStateLength(settings["storage.fixedvectorsize"].asUnsignedValue());
        //        }
        //    }
        mc.setSettings(settings);
        mc.getStorage().setSettings(settings);

        mc.go();

        auto& endStates = mc.getEndStates();
        if(endStates.size() > 0) {
            std::stringstream ss;
            ss << "End states (" << endStates.size() << ")";
            out.reportAction(ss.str());
            out.indent();
            size_t endStatesError = 0;
            Storage& storage = mc.getStorage();
            std::vector<typename Storage::StateSlot> buffer;
            for(auto const& s: endStates) {
                size_t stateLength = storage.determineLength(s);
                buffer.reserve(stateLength);
                mc.getState(s, buffer.data(), true);
                printer.writeEndState(model, s, Storage::FullState::createExternal(true, stateLength, buffer.data()));
                std::stringstream sss;
                sss << s;

                // TODO: make generic
                if(buffer.data()[6]) {
                    endStatesError++;
                }
            }
            size_t endStatesOK = endStates.size() - endStatesError;
            if(endStatesOK) {
                std::stringstream sss;
                sss << endStatesOK << " end states OK (first process/thread terminated correctly)";
                out.reportSuccess(sss.str());
            }
            if(endStatesError) {
                std::stringstream sss;
                sss << endStatesError << " end states with issues";
                out.reportSuccess(sss.str());
            }
            out.outdent();
        } else {
            out.reportAction("No end states");
        }
        printer.finish();

    } else {
        out.reportError("Failed to load model");
    }
    f.close();
}

template<typename Storage, template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goSelectPrinter(MessageFormatter& out, std::string fileName) {
    Settings& settings = Settings::global();
    if(settings["listener"].asString() == "dotall") {
        goDMC<Storage, llmc::statespace::DotPrinter, ModelChecker>(out, fileName);
    } else if(settings["listener"].asString() == "dotend") {
        goDMC<Storage, llmc::statespace::DotPrinterEndOnly, ModelChecker>(out, fileName);
    } else {
        goDMC<Storage, llmc::statespace::VoidPrinter, ModelChecker>(out, fileName);
    }
}

template<template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goSelectStorage(MessageFormatter& out, std::string fileName) {
    Settings& settings = Settings::global();

    if(settings["storage"].asString() == "stdmap") {
        goSelectPrinter<llmc::storage::StdMap, ModelChecker>(out, fileName);
    } else if(settings["storage"].asString() == "cchm") {
        goSelectPrinter<llmc::storage::cchm, ModelChecker>(out, fileName);
    } else if(settings["storage"].asString() == "treedbs_stdmap") {
        goSelectPrinter<llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, ModelChecker>(out, fileName);
    } else if(settings["storage"].asString() == "treedbs_cchm") {
        goSelectPrinter<llmc::storage::TreeDBSStorage<llmc::storage::cchm>, ModelChecker>(out, fileName);
    } else if(settings["storage"].asString() == "treedbsmod") {
        goSelectPrinter<llmc::storage::TreeDBSStorageModified, ModelChecker>(out, fileName);
    } else if(settings["storage"].asString() == "dtree") {
        goSelectPrinter<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(out, fileName);
//    } else if(settings["storage"].asString() == "dtree2") {
//        goDMC<llmc::storage::DTree2Storage<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>,SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(fileName);
    } else {
        out.reportError("No such storage component: " + settings["storage"].asString());
    }
}

void go(MessageFormatter& out, std::string fileName) {
    Settings& settings = Settings::global();

    if(settings["mc"].asString() == "multicore_simple") {
        goSelectStorage<MultiCoreModelCheckerSimple>(out, fileName);
    } else if(settings["mc"].asString() == "multicore_bitbetter") {
        goSelectStorage<MultiCoreModelChecker>(out, fileName);
    } else if(settings["mc"].asString() == "singlecore_simple") {
        goSelectStorage<SingleCoreModelChecker>(out, fileName);
    } else {
        out.reportError("No such model checker: " + settings["mc"].asString());
    }
}

int main(int argc, char* argv[]) {

    Settings& settings = Settings::global();

    settings["threads"] = 0;
    settings["mc"] = "multicore_bitbetter";
    settings["storage"] = "dtree";
    settings["storage.stats"] = 0;
    settings["storage.bars"] = 128;

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

    int c = 0;
    while ((c = getopt(argc, argv, "m:s:t:-:")) != -1) {
//    while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
        switch(c) {
            case 't':
                if(optarg) {
                    settings["threads"] = std::stoi(optarg);
                }
                break;
            case 'm':
                if(optarg) {
                    settings["mc"] = std::string(optarg);
                }
                break;
            case 's':
                if(optarg) {
                    settings["storage"] = std::string(optarg);
                }
                break;
            case '-':
                settings.insertKeyValue(optarg);
        }
    }

    int htindex = optind;

    if(htindex >= argc) {
        out.reportError("No input file specified");
        exit(-1);
    }

    File input(System::getArgument(htindex));
    input.fix();
    File output_ll = input.newWithExtension("dmc.ll");
    File output_o = input.newWithExtension("dmc.o");
    File output_so = input.newWithExtension("so");
    File output_dot = input.newWithExtension("dot");
    File output_png = input.newWithExtension("png");

    if(input.getFileExtension()=="so") {
        go(out, input.getFileRealPath());
        return 0;
    }

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

    // .ll -> .dmc.ll
    out.reportAction("Translating LLVM IR...");
    llmc::ll2dmc translator(out);
    translator.init(input, settings.getSubSection("ll2dmc"));
    auto r = translator.translate();
    translator.writeTo(output_ll);
    if(!r) {
        out.reportError("Translation failed");
        exit(1);
    }
    if(out.getErrors()) {
        out.reportErrors();
        exit(1);
    }

    // .dmc.ll -> .o
    FileSystem::remove(output_o);
    if(compile(bin_llc, output_ll, output_o, out)) {
        out.reportError("Compilation failed");
        exit(1);
    }
    out.reportSuccess("Compilation successful");

    // .o -> .so
    if(link(bin_cc, output_o, output_so, out)) {
        out.reportError("Linking failed");
        exit(1);
    }
    out.reportSuccess("Linking successful");

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

    out.reportAction("Exploring the state space using the following settings");
    out.indent();
    out.reportNote("search core:   " + settings["mc"].asString());
    out.reportNote("state storage: " + settings["storage"].asString());
    out.outdent();
    go(out, output_so.getFileRealPath());

    return 0;
}
