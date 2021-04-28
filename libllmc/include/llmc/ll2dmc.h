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
