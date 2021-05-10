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

#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>

namespace llmc {

class StateManager {
private:
    Value* userContext;
    LLDMCModelGenerator* gen;
    SVType* type;
public:

    StateManager(Value* userContext, LLDMCModelGenerator* gen, SVType* type)
            : userContext(userContext), gen(gen), type(type) {
    }

    Value* upload(Value* data, Value* length);
    Value* uploadBytes(Value* data, Value* lengthInBytes);

    Value* upload(Value* stateID, Value* data, Value* length);
    Value* uploadBytes(Value* stateID, Value* data, Value* lengthInBytes);

    Value* download(Value* stateID, Value* data);
    Value* download(Value* stateID);

    Value* delta(Value* stateID, Value* offset, Value* data);
    Value* delta(Value* stateID, Value* offset, Value* length, Value* data);
    Value* deltaBytes(Value* stateID, Value* offsetInBytes, Value* data);
    Value* deltaBytes(Value* stateID, Value* offsetInBytes, Value* lengthInBytes, Value* data);

    Value* downloadPart(Value* stateID, Value* offset, Value* length, Value* data);
    Value* downloadPartBytes(Value* stateID, Value* offsetInBytes, Value* lengthInBytes, Value* data);

    Value* getLength(Value* stateID);
};

} // namespace llmc