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