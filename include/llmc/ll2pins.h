#pragma once

#include <llmc/LLPinsGenerator.h>

namespace llmc {

int ll2pins(File const& input, File const& output, MessageFormatter& out) {
    llvm::LLVMContext llvmctx;
    llvm::SMDiagnostic Err;

    auto llvmModel = llvm::getLazyIRFileModule(System::getArgument(1), Err, llvmctx);
    if(!llvmModel) {
        raw_os_ostream out(std::cerr);
        Err.print("llvm model", out, true);
        return 1;
    }

    llmc::LLPinsGenerator gen(std::move(llvmModel), out);

    gen.pinsify();
    gen.writeTo(output.getFilePath());

    return 0;
}

}
