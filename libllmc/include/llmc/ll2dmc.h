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

#pragma once

#include <libfrugi/Settings.h>
#include <llmc/LLDMCModelGenerator.h>

using namespace libfrugi;

namespace llmc {

class ll2dmc {
public:
    ll2dmc(MessageFormatter& out): _gen(nullptr), _llvmModel(nullptr), _out(out) {
    }

    bool init(File const& input, Settings const& settings) {
        _llvmModel = llvm::getLazyIRFileModule(input.getFileRealPath(), Err, llvmctx);
        if(!_llvmModel) {
            string s;
            raw_string_ostream rsoout(s);
            Err.print("llvm model", rsoout, true);
            _out.reportError(rsoout.str());
            return false;
        }
        _gen = new LLDMCModelGenerator(std::move(_llvmModel), _out);
        _gen->enableDebugChecks();
        if(settings["assume_nonatomic_collapsable"].isOn()) {
            _gen->assumeNonAtomicCollapsable();
        }
        return true;
    }

    bool translate() {
        return _gen->pinsify();
    }

    void writeTo(File const& output) {
        _gen->writeTo(output.getFilePath());
    }

private:
    llvm::LLVMContext llvmctx;
    llvm::SMDiagnostic Err;
    LLDMCModelGenerator* _gen;
    std::unique_ptr<Module> _llvmModel;
    MessageFormatter& _out;
};

}
