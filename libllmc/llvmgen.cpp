#include "config.h"
#include "llmc/llvmgen.h"

void llvmgen::BBComment(IRBuilder<>& builder, std::string const& comment) {
    auto bb = BasicBlock::Create(builder.GetInsertBlock()->getContext(), comment, builder.GetInsertBlock()->getParent());
    builder.CreateBr(bb);
    builder.SetInsertPoint(bb);
}
