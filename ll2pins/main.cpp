#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <llmc/ll2dmc.h>
#include <llmc/LLDMCModelGenerator.h>
#include <libfrugi/System.h>
//#include <llmc/xgraph.h>

using namespace llmc;

int main(int argc, char* argv[]) {

    MessageFormatter out(std::cout);

    Settings& settings = Settings::global();

    // init libfrugi
    System::init(argc, argv);

    if(argc != 3) {
        std::cout << "Usage: " << System::getArgument(0) << " <input.ll> <output.ll>" << std::endl;
        return 0;
    }

    File input(System::getArgument(1));
    File output(System::getArgument(2));

    if(!FileSystem::hasAccessTo(input, R_OK)) {
        std::cout << "Cannot read input file" << std::endl;
        exit(1);
    }
    if(FileSystem::isDir(output)) {
        std::cout << "Output file is a directory" << std::endl;
        exit(1);
    }
    if(!FileSystem::canCreateOrModify(output)) {
        std::cout << "Cannot create or modify output file" << std::endl;
        exit(1);
    }

    int c = 0;
    while ((c = getopt(argc, argv, "m:s:t:-:")) != -1) {
        switch(c) {
            case '-':
                settings.insertKeyValue(optarg);
                break;
        }
    }

    llmc::ll2dmc translator(out);
    translator.init(input, settings.getSubSection("ll2dmc"));
    translator.translate();
    translator.writeTo(output);

    return 0;
}
