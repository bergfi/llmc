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

void printHelp(MessageFormatter& out) {
    out.notify("llmc [options] [input LLVM IR file]");
    out.message("    A stateful multi-core model checker of LLVM IR.");
    out.message("");
    out.notify ("General Options:");
    out.message("  -h, --help                  Show this help.");
    out.message("  --color                     Use colored messages.");
    out.message("  --no-color                  Do not use colored messages.");
    out.message("  --version                   Print version info and quit.");
    out.message("");
    out.notify ("Debug Options:");
    out.message("  --verbose=x                 Set verbosity to x, -1 <= x <= 5.");
    out.message("  -v, --verbose               Increase verbosity. Up to 5 levels.");
    out.message("  -q                          Decrease verbosity.");
    out.message("");
    out.notify("DMC Model Checker Options:");
    out.message("  -m MC, --mc MC              Use MC model checker. Options for MC: ");
    out.message("                                - singlecore_simple: simple, single-core");
    out.message("                                - multicore_simple: multi-core, single-queue");
    out.message("                                > multicore_bitbetter: multi-core, work-sharing");
    out.message("  -s S, --storage S           Use S state storage. Options for S: ");
    out.message("                                > dtree: DTree compression tree");
    out.message("                                - treedbsmod: TreeDBS tree, states padded");
    out.message("                                - treedbs_cchm: TreeDBS + CCHM");
    out.message("                                - cchm: Concurrent Chaining Hash Map");
    out.message("                                - stdmap: std::unordered_map");
    out.message("  -t T, --threads T           Use T threads to model check, 0 for auto, default");
    out.message("  --listener=L                Use listener L to action upon exploration:");
    out.message("                                - dotall: all states/transistions to a DOT file");
    out.message("                                - dotend: Write end states to a DOT file");
    out.message("                                > none: Do not listen to changes.");
    out.message("");
    out.notify("DMC Model Checker Miscellaneous Options:");
    out.message("  --storage.stats=on          Enable storage statistics");
    out.message("  --storage.bars=N            Storage statistics uses N bars. Default 128.");
    out.message("  --storage.hashmap_scale=N   Sizes of both hashmaps. Default 28.");
    out.message("  --storage.hashmaproot_scale=N Size of hashmap for root nodes. Default 28.");
    out.message("  --storage.hashmapdata_scale=N Size of hashmap for data nodes. Default 28.");
    out.message("  --listener.writestate=on    Enable writing complete states");
    out.message("  --listener.writesubstate=on Enable writing sub-states, not only root-states");
    out.message("");
}

void printVersion(MessageFormatter& out) {
    printf("llmc %s\n", CompileOptions::LLMC_VERSION);
    printf("Copyright © 2021 Freark van der Berg\n"
           "This is free software; see the source for copying conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

int main(int argc, char* argv[]) {

    Settings& settings = Settings::global();

    settings["threads"] = 0;
    settings["mc"] = "multicore_bitbetter";
    settings["storage"] = "dtree";
    settings["storage.stats"] = 0;
    settings["storage.bars"] = 128;

    int verbosity = 0;
    bool doPrintHelp = false;
    bool doPrintVersion = false;

    MessageFormatter out(std::cout);
    out.setAutoFlush(true);
    out.useColoredMessages(true);

    Shell::messageFormatter = &out;

    // init libfrugi
    System::init(argc, argv);

    int c = 0;
    while ((c = getopt(argc, argv, "hm:qs:t:v-:")) != -1) {
//    while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
        switch(c) {
            case 'h':
                doPrintHelp = true;
                break;
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
            case 'q':
                verbosity--;
                break;
            case 's':
                if(optarg) {
                    settings["storage"] = std::string(optarg);
                }
                break;
            case 'v':
                verbosity++;
                break;
            case '-':
                if(!strcasecmp(optarg, "help")) {
                    doPrintHelp = true;
                } else if(!strcasecmp(optarg, "no-color")) {
                    out.useColoredMessages(false);
                } else if(!strcasecmp(optarg, "color")) {
                    out.useColoredMessages(true);
                } else if(!strcasecmp(optarg, "verbose")) {
                    int len = strlen(optarg);
                    if(len > 7 && optarg[7] == '=') {
                        verbosity = atoi(optarg + 8);
                    } else {
                        ++verbosity;
                    }
                } else if(!strcasecmp(optarg, "version")) {
                    doPrintVersion = true;
                } else {
                    settings.insertKeyValue(optarg);
                }
        }
    }

    out.setVerbosity(verbosity);

    if(doPrintHelp) {
        printHelp(out);
        exit(0);
    }
    if(doPrintVersion) {
        printVersion(out);
        exit(0);
    }

    // Find binaries
    File bin_llc;
    File bin_cc;
    File bin_ltsmin;
    File bin_dot;

    findBinary("llc", out, bin_llc);
    if(findBinary("gcc", out, bin_cc)) {
        findBinary("clang", out, bin_cc);
    }
    findBinary("dot", out, bin_dot);

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
