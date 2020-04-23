
#include <fstream>
#include <llmc/model.h>
#include <llmc/statespace/listener.h>
#include "libfrugi/include/libfrugi/Settings.h"
#include <llmc/modelcheckers/interface.h>
#include <llmc/modelcheckers/multicoresimple.h>
#include <llmc/modelcheckers/singlecore.h>
#include <llmc/modelcheckers/multicore_bitbetter.h>
#include <llmc/statespace/listener.h>
#include <llmc/storage/interface.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/dtree2.h>
#include <llmc/storage/stdmap.h>
#include <llmc/storage/cchm.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>
#include "StateIdentifier.h"
#include "promelamodels/bakery.6.prom.dmcmodel.h"
#include "promelamodels/phils.5.prom.dmcmodel.h"

template<typename Storage, template <typename, typename,template<typename,typename> typename> typename ModelChecker>
void goDMCModel() {
    Settings& settings = Settings::global();
    using MC = ModelChecker<VModel<llmc::storage::StorageInterface>, Storage, llmc::statespace::DotPrinter>;

    std::ofstream f;
    f.open("out.dot", std::fstream::trunc);

    VModel<llmc::storage::StorageInterface>* model = nullptr;
    std::string modelName = settings["model"].asString();
    if(!modelName.empty()) {
        dlerror();
        auto handle = dlopen(modelName.c_str(), RTLD_NOW);
        if(char* err = dlerror()) {
            printf("Error opening model %s: %s\n", modelName.c_str(), err);
            return;
        }

        VModel<llmc::storage::StorageInterface>* (*func)() = (VModel<llmc::storage::StorageInterface>* (*)())dlsym(handle, "getModel");
        if(char* err = dlerror()) {
            printf("Error locating getModel() in model %s: %s\n", modelName.c_str(), err);
            return;
        }
        model = func();
    } else {
        std::cout << "Wrong or no model selected: " << modelName << std::endl;
    }
    if(model) {
        llmc::statespace::DotPrinter<MC, VModel<llmc::storage::StorageInterface>
        > printer(f);
        MC mc(model, printer);
        mc.getStorage().setSettings(settings);
        mc.setSettings(settings);
        mc.go();
    }
}

template<typename T>
struct HashCompareMurmur {
    static constexpr uint64_t hashForZero = 0x7208f7fa198a2d81ULL;
    static constexpr uint64_t seedForZero = 0xc6a4a7935bd1e995ULL*8ull;

    __attribute__((always_inline))  // he he
    bool equal( const T& j, const T& k ) const {
        return j == k;
    }

    __attribute__((always_inline))
    size_t hash( const T& k ) const {
        return MurmurHash64(&k, sizeof(T), seedForZero);
    }
};

template<template <typename, typename, template<typename,typename> typename> typename ModelChecker>
void goSelectStorage() {
    Settings& settings = Settings::global();

    if(settings["storage"].asString() == "stdmap") {
        goDMCModel<llmc::storage::StdMap, ModelChecker>();
    } else if(settings["storage"].asString() == "cchm") {
        goDMCModel<llmc::storage::cchm, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_stdmap") {
        goDMCModel<llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbs_cchm") {
        goDMCModel<llmc::storage::TreeDBSStorage<llmc::storage::cchm>, ModelChecker>();
    } else if(settings["storage"].asString() == "treedbsmod") {
        goDMCModel<llmc::storage::TreeDBSStorageModified, ModelChecker>();
    } else if(settings["storage"].asString() == "dtree") {
        goDMCModel<llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>();
//	} else if(settings["storage"].asString() == "dtree2") {
//		goPINS<llmc::storage::DTree2Storage<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>,SeparateRootSingleHashSet<HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, ModelChecker>(fileName);
    } else {
        std::cout << "Wrong or no storage selected: " << settings["storage"].asString() << std::endl;
    }
}

void go() {
    Settings& settings = Settings::global();

    if(settings["mc"].asString() == "multicore_simple") {
        goSelectStorage<MultiCoreModelCheckerSimple>();
    } else if(settings["mc"].asString() == "multicore_bitbetter") {
        goSelectStorage<MultiCoreModelChecker>();
    } else if(settings["mc"].asString() == "singlecore_simple") {
        goSelectStorage<SingleCoreModelChecker>();
    } else {
        std::cout << "Wrong or no model checker selected: " << settings["mc"].asString() << std::endl;
    }
}

int main(int argc, char** argv) {

    Settings& settings = Settings::global();
    int c = 0;
    while ((c = getopt(argc, argv, "m:s:t:-:")) != -1) {
//	while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
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

    go();

}
