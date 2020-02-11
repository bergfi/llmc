#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <libfrugi/MessageFormatter.h>
#include <libfrugi/Settings.h>
#include <libfrugi/Shell.h>
#include <libfrugi/System.h>
#include <llmc/ll2pins.h>
#include <llmc/ssgen.h>
#include <llmc/modelcheckers/interface.h>
#include <llmc/modelcheckers/multicoresimple.h>
#include <llmc/modelcheckers/singlecore.h>
#include <llmc/modelcheckers/multicore.h>
#include <llmc/statespace/listener.h>
#include <llmc/storage/interface.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/stdmap.h>
#include <llmc/storage/cchm.h>
#include <llmc/storage/treedbs.h>
#include <sstream>
#include <llmc/murmurhash.h>

#include <llmc/models/PINSModel.h>
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
                   + " -O3"
                   + " " + libLLMCOSObject.getFilePath()
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
        return MurmurHash64(&k, 8, seedForZero);
    }
};

void goOld(std::string soFile) {
    model* model = model_pins_so::get(soFile);
    assert(model);
    ssgen_st ss(model);
    ss.go();
}

template<typename Storage, template <typename, typename,template<typename,typename> typename> typename ModelChecker>
void goPINS(std::string soFile) {
    Settings& settings = Settings::global();

    ofstream f;
    f.open("out.dot", std::fstream::trunc);

    using MC = ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>;

//    PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>* model = PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>::get(soFile);
//    llmc::statespace::DotPrinter< ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>
//                                , PINSModel<ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter>>
//                                > printer(f);
//    ModelChecker<PINSModel, Storage, llmc::statespace::DotPrinter> mc(model, printer);

    VModel<llmc::storage::StorageInterface>* model = PINSModel::get(soFile);
    llmc::statespace::DotPrinter< MC
                                , VModel<llmc::storage::StorageInterface>
                                > printer(f);
    MC mc(model, printer);

//    VModel<llmc::storage::StorageInterface>* model = new LLVMModel();
//    llmc::statespace::DotPrinter< ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>
//                                , VModel<llmc::storage::StorageInterface>
//                                > printer(f);
//    ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter> mc(model, printer);

    if constexpr(Storage::stateHasFixedLength()) {
        if(settings["storage.fixedvectorsize"].asUnsignedValue() > 0) {
            mc.getStorage().setMaxStateLength(settings["storage.fixedvectorsize"].asUnsignedValue());
        }
    }

    mc.go();
    f.close();
}

template<template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goSelectStorage(std::string fileName) {
    Settings& settings = Settings::global();

    if(settings["storage"].asString() == "stdmap") {
        goPINS<llmc::storage::StdMap, ModelChecker>(fileName);
    } else if(settings["storage"].asString() == "cchm") {
        goPINS<llmc::storage::cchm, ModelChecker>(fileName);
    } else if(settings["storage"].asString() == "treedbs") {
        goPINS<llmc::storage::TreeDBSStorage, ModelChecker>(fileName);
    } else if(settings["storage"].asString() == "dtree") {
        goPINS<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(fileName);
    } else {
    }
}

void go(std::string fileName) {
    Settings& settings = Settings::global();

    if(settings["mc"].asString() == "multicore_simple") {
        goSelectStorage<MultiCoreModelCheckerSimple>(fileName);
    } else if(settings["mc"].asString() == "multicore_bitbetter") {
        goSelectStorage<MultiCoreModelChecker>(fileName);
    } else if(settings["mc"].asString() == "singlecore_simple") {
        goSelectStorage<SingleCoreModelChecker>(fileName);
    } else {
    }
}

int main(int argc, char* argv[]) {

    Settings& settings = Settings::global();

    settings["threads"] = 0;
    settings["mc"] = "singlecore_simple";
    settings["storage"] = "hashmap";
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
    File output_ll = input.newWithExtension("pins.ll");
    File output_o = input.newWithExtension("pins.o");
    File output_so = input.newWithExtension("so");
    File output_dot = input.newWithExtension("dot");
    File output_png = input.newWithExtension("png");

    if(input.getFileExtension()=="so") {
        go(input.getFileRealPath());
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

    // .ll -> .pins.ll
    out.reportAction("Pinsifying...");
    if(ll2pins(input, output_ll, out)) {
        out.reportError("Pinsifying failed");
        exit(1);
    }

    // .pins.ll -> .o
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

    go(output_so.getFileRealPath());

    std::cout << "Model checker:   " << settings["mc"].asString() << std::endl;
    std::cout << "Storage:         " << settings["storage"].asString() << std::endl;

    return 0;
}
