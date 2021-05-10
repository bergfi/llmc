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
