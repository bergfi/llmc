#include <llmc/generation/StateManager.h>
#include <llmc/LLDMCModelGenerator.h>

namespace llmc {

static void checkIsMultipleOf(LLDMCModelGenerator* gen, Value* v, Value* multipleOf, std::string ass, std::string function) {
    if(v->getType()->isIntegerTy()) {
        v = gen->builder.CreateIntCast(v, gen->t_int64, false);
    } else if(v->getType()->isPointerTy()) {
        v = gen->builder.CreatePtrToInt(v, gen->t_int64);
    }
    v = gen->builder.CreateAnd(v, gen->builder.CreateIntCast(multipleOf, gen->t_int64, false));
    auto cmp = gen->builder.CreateICmpEQ(v, ConstantInt::get(gen->t_int64, 0));
//    auto BBEnd = BasicBlock::Create(gen->ctx, "is_not_multiple_of_end", gen->builder.GetInsertBlock()->getParent());
//    llvmgen::If2 genIf(gen->builder, cmp, "is_not_multiple_of", BBEnd);
//    genIf.startTrue();

    auto f_fatal = gen->llmcvm_func("__LLMCOS_Assert_Fatal");
    auto t_fatal = f_fatal->getFunctionType();

    gen->builder.CreateCall(f_fatal, {
          gen->builder.CreateIntCast(cmp, t_fatal->getParamType(0), false)
        , gen->builder.CreatePointerCast(gen->generateGlobalString(ass), t_fatal->getParamType(1))
        , gen->builder.CreatePointerCast(gen->generateGlobalString(function), t_fatal->getParamType(2))
        , ConstantInt::get(t_fatal->getParamType(3), __LINE__)
        , gen->builder.CreatePointerCast(gen->generateGlobalString(v->getName().str()), t_fatal->getParamType(4))
    });
//    genIf.endTrue();
//    genIf.startFalse();
//    genIf.endFalse();
//    genIf.finally();

}

Value* StateManager::upload(Value* data, Value* length) {
    auto& dmc_insert = gen->f_dmc_insert;
    assert(dmc_insert);
    assert(data);
    assert(length);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_insert->getFunctionType();
    assert(data->getType()->isPointerTy());
    assert(length->getType()->isIntegerTy());
    return gen->builder.CreateCall(dmc_insert, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
                                               , gen->builder.CreatePointerCast(data, FType->getParamType(1))
                                               , gen->builder.CreateIntCast(length, FType->getParamType(2), false)
                                               , ConstantInt::get(FType->getParamType(3), isRoot)
                                               });
}

Value* StateManager::uploadBytes(Value* data, Value* lengthInBytes) {
    auto& dmc_insert = gen->f_dmc_insertBytes;
    assert(dmc_insert);
    assert(data);
    assert(data->getType()->isPointerTy());
    assert(lengthInBytes);
    assert(lengthInBytes->getType()->isIntegerTy());

    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_insert->getFunctionType();
    return gen->builder.CreateCall(dmc_insert, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
            , gen->builder.CreatePointerCast(data, FType->getParamType(1))
            , gen->builder.CreateIntCast(lengthInBytes, FType->getParamType(2), false)
            , ConstantInt::get(FType->getParamType(3), isRoot)
    });
}

Value* StateManager::upload(Value* stateID, Value* data, Value* length) {
    auto& dmc_transition = gen->f_dmc_transition;
    assert(dmc_transition);
    assert(data);
    assert(length);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_transition->getFunctionType();
    assert(data->getType()->isPointerTy());
    assert(length->getType()->isIntegerTy());
    return gen->builder.CreateCall(dmc_transition, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
            , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
            , gen->builder.CreatePointerCast(data, FType->getParamType(2))
            , gen->builder.CreateIntCast(length, FType->getParamType(3), false)
            , ConstantInt::get(FType->getParamType(4), isRoot)
    });
}

Value* StateManager::uploadBytes(Value* stateID, Value* data, Value* lengthInBytes) {
    assert(lengthInBytes);
    assert(lengthInBytes->getType()->isIntegerTy());
    checkIsMultipleOf(gen, lengthInBytes, ConstantInt::get(gen->t_int64, 4), "Length is not multiple of 4", "uploadBytes");
    auto length = gen->builder.CreateUDiv(lengthInBytes, ConstantInt::get(lengthInBytes->getType(), 4));
    return upload(stateID, data, length);
}

Value* StateManager::download(Value* stateID, Value* data) {
    auto& dmc_get = gen->f_dmc_get;
    assert(dmc_get);
    assert(stateID);
    assert(data);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_get->getFunctionType();
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    assert(data->getType()->isPointerTy());
    return gen->builder.CreateCall(dmc_get, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
                                            , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
                                            , gen->builder.CreatePointerCast(data, FType->getParamType(2))
                                            , ConstantInt::get(FType->getParamType(3), isRoot)
                                            });
}

Value* StateManager::download(Value* stateID) {
    assert(stateID);
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    auto length = getLength(stateID);
    Value* data = gen->builder.CreateAlloca(gen->t_int, length);
    if(type) {
        data =  gen->builder.CreatePointerCast(data, type->getLLVMType()->getPointerTo());
    }
    download(stateID, data);
    return data;
}

Value* StateManager::delta(Value* stateID, Value* offset, Value* data) {
    assert(stateID);
    assert(offset);
    assert(data);
    auto pData = gen->builder.CreateAlloca(data->getType());
    gen->builder.CreateStore(data, pData);
    return delta(stateID, offset, gen->generateSizeOf(data->getType()), pData);
}

Value* StateManager::delta(Value* stateID, Value* offset, Value* length, Value* data) {
    auto& dmc_delta = gen->f_dmc_delta;
    assert(dmc_delta);
    assert(stateID);
    assert(offset);
    assert(length);
    assert(data);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_delta->getFunctionType();
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    assert(offset->getType()->isIntegerTy());
    assert(length->getType()->isIntegerTy());
    assert(data->getType()->isPointerTy());
    auto V = gen->builder.CreateCall(dmc_delta, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
                                                , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
                                                , gen->builder.CreateIntCast(offset, FType->getParamType(2), false)
                                                , gen->builder.CreatePointerCast(data, FType->getParamType(3))
                                                , gen->builder.CreateIntCast(length, FType->getParamType(4), false)
                                                , ConstantInt::get(FType->getParamType(5), isRoot)
                                                });
    V->setName("delta");
    return V;
}

Value* StateManager::deltaBytes(Value* stateID, Value* offsetInBytes, Value* data) {
    assert(stateID);
    assert(offsetInBytes);
    assert(data);
    auto pData = gen->builder.CreateAlloca(data->getType());
    gen->builder.CreateStore(data, pData);
    return deltaBytes(stateID, offsetInBytes, gen->generateSizeOf(data->getType()), pData);
}

Value* StateManager::deltaBytes(Value* stateID, Value* offsetInBytes, Value* lengthInBytes, Value* data) {
    auto& dmc_delta = gen->f_dmc_deltaBytes;
    assert(dmc_delta);
    assert(stateID);
    assert(offsetInBytes);
    assert(data);
    assert(lengthInBytes);
    assert(lengthInBytes->getType()->isIntegerTy());

    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_delta->getFunctionType();
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    assert(offsetInBytes->getType()->isIntegerTy());
    assert(lengthInBytes->getType()->isIntegerTy());
    assert(data->getType()->isPointerTy());
    auto V = gen->builder.CreateCall(dmc_delta, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
            , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
            , gen->builder.CreateIntCast(offsetInBytes, FType->getParamType(2), false)
            , gen->builder.CreatePointerCast(data, FType->getParamType(3))
            , gen->builder.CreateIntCast(lengthInBytes, FType->getParamType(4), false)
            , ConstantInt::get(FType->getParamType(5), isRoot)
    });
    V->setName("deltaB");
    return V;

}

Value* StateManager::downloadPart(Value* stateID, Value* offset, Value* length, Value* data) {
    auto& dmc_getpart = gen->f_dmc_getpart;
    assert(dmc_getpart);
    assert(stateID);
    assert(offset);
    assert(length);
    assert(data);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_getpart->getFunctionType();
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    assert(offset->getType()->isIntegerTy());
    assert(length->getType()->isIntegerTy());
    assert(data->getType()->isPointerTy());
    auto V = gen->builder.CreateCall(dmc_getpart, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
            , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
            , gen->builder.CreateIntCast(offset, FType->getParamType(2), false)
            , gen->builder.CreatePointerCast(data, FType->getParamType(3))
            , gen->builder.CreateIntCast(length, FType->getParamType(4), false)
            , ConstantInt::get(FType->getParamType(5), isRoot)
    });
    V->setName("downloadPart");
    return V;
}

Value* StateManager::downloadPartBytes(Value* stateID, Value* offsetInBytes, Value* lengthInBytes, Value* data) {
    auto& dmc_getpart = gen->f_dmc_getpartBytes;
    assert(dmc_getpart);
    assert(lengthInBytes);
    assert(lengthInBytes->getType()->isIntegerTy());
    assert(stateID);
    assert(offsetInBytes);
    assert(lengthInBytes);
    assert(data);
    bool isRoot = gen->lts.getRootType() == type;
    auto FType = dmc_getpart->getFunctionType();
    if(stateID->getType()->isPointerTy()) {
        stateID = gen->builder.CreateLoad(stateID);
    }
    assert(stateID->getType()->isIntegerTy());
    assert(offsetInBytes->getType()->isIntegerTy());
    assert(lengthInBytes->getType()->isIntegerTy());
    assert(data->getType()->isPointerTy());
    auto V = gen->builder.CreateCall(dmc_getpart, { gen->builder.CreatePointerCast(userContext, FType->getParamType(0))
            , gen->builder.CreateIntCast(stateID, FType->getParamType(1), false)
            , gen->builder.CreateIntCast(offsetInBytes, FType->getParamType(2), false)
            , gen->builder.CreatePointerCast(data, FType->getParamType(3))
            , gen->builder.CreateIntCast(lengthInBytes, FType->getParamType(4), false)
            , ConstantInt::get(FType->getParamType(5), isRoot)
    });
    V->setName("downloadPartB");
    return V;
}


Value* StateManager::getLength(Value* stateID) {
    return gen->builder.CreateLShr(stateID, 40);
}

} // namespace llmc