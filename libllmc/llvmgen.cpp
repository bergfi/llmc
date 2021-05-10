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
#include "llmc/llvmgen.h"

void llvmgen::BBComment(IRBuilder<>& builder, std::string const& comment) {
    auto bb = BasicBlock::Create(builder.GetInsertBlock()->getContext(), comment, builder.GetInsertBlock()->getParent());
    builder.CreateBr(bb);
    builder.SetInsertPoint(bb);
}
