#include <llmc/generation/ProcessStack.h>
#include <llmc/LLPinsGenerator.h>

namespace llmc {

void ProcessStack::init() {
    // previous pc, register frame, register index where to store result, previous chunkID
    t_frame = StructType::create(gen->ctx, {gen->t_int, gen->t_chunkid, gen->t_int, gen->t_chunkid}, "t_stackframe", true);
    t_framep = t_frame->getPointerTo();
}

Value* ProcessStack::getEmptyStack(GenerationContext* gctx) {
//    std::string name = "emptystack_chunkid";
//    if(!gen->pinsModule->getGlobalVariable(name, true)) {
//
//        // Create global variable
//        assert(!GV_emptyStack);
//        GV_emptyStack = new GlobalVariable(*gen->pinsModule, gen->t_chunkid, false, GlobalValue::LinkageTypes::InternalLinkage, ConstantInt::get(gen->t_chunkid, 0), name);
//        assert(GV_emptyStack);
//
//        // Create an emoty stackframe with all 0's
//        ChunkMapper cm_stack(gctx, gen->type_stack);
////        auto emptyStack = gen->builder.CreateAlloca(t_frame);
//        Value* emptyStack = gen->builder.CreateAlloca(IntegerType::get(gen->ctx, 8), gen->generateAlignedSizeOf(t_frame));
//        emptyStack = gen->builder.CreatePointerCast(emptyStack, t_frame->getPointerTo());
//        gen->builder.CreateMemSet( emptyStack
//                                , ConstantInt::get(gen->t_int8, 0)
//                                , gen->generateAlignedSizeOf(t_frame)
//                                , emptyStack->getPointerAlignment(gen->pinsModule->getDataLayout())
//                                );
//        auto newStackChunkID = cm_stack.generatePut(gen->generateAlignedSizeOf(t_frame), emptyStack);
//
//        // Store it in the global
//        gen->builder.CreateStore(newStackChunkID, GV_emptyStack);
//    }
//
//    // Load the global
//    return gen->builder.CreateLoad(GV_emptyStack);
    return ConstantInt::get(gen->t_chunkid, 0);
}

Value* ProcessStack::generateNewFrame(Value* oldPC, Value* rframe_chunkID, CallInst* callSite, Value* prevFrame_chunkID) {

    // Find register index of the register where to put the result
    Value* regOffset;
    auto iIdx = gen->valueRegisterIndex.find(callSite);
    if(iIdx == gen->valueRegisterIndex.end()) {
        regOffset = ConstantInt::get(gen->t_int, 0);
    } else {
        auto idx = iIdx->second;

        // Determine the byte offset within the register to this index
        auto regType = PointerType::get(gen->registerLayout[callSite->getParent()->getParent()].registerLayout, 0);
        auto regs = gen->builder.CreateBitOrPointerCast(ConstantPointerNull::get(regType), regType);
        regOffset = gen->builder.CreateGEP( regs
                                               , { ConstantInt::get(gen->t_int, 0)
                                                 , ConstantInt::get(gen->t_int, idx)
                                                  }
        );
        regOffset = gen->builder.CreatePtrToInt(regOffset, gen->t_int);
    }

    // Create a new frame
//    auto frame = gen->builder.CreateAlloca(t_frame);
    Value* frame = gen->builder.CreateAlloca(IntegerType::get(gen->ctx, 8), gen->generateAlignedSizeOf(t_frame));
    frame = gen->builder.CreatePointerCast(frame, t_frame->getPointerTo());
    auto prevPC = gen->builder.CreateGEP( frame
                                       , { ConstantInt::get(gen->t_int, 0)
                                         , ConstantInt::get(gen->t_int, 0)
                                         }
                                       );
    auto prevRegsChunkID = gen->builder.CreateGEP( frame
                                                , { ConstantInt::get(gen->t_int, 0)
                                                  , ConstantInt::get(gen->t_int, 1)
                                                  }
                                                );
    auto resultRegister = gen->builder.CreateGEP( frame
                                               , { ConstantInt::get(gen->t_int, 0)
                                                 , ConstantInt::get(gen->t_int, 2)
                                                 }
                                               );
    auto prevFrameChunkID = gen->builder.CreateGEP( frame
                                                 , { ConstantInt::get(gen->t_int, 0)
                                                   , ConstantInt::get(gen->t_int, 3)
                                                   }
                                                 );
    gen->builder.CreateStore(oldPC, prevPC);
    gen->builder.CreateStore(rframe_chunkID, prevRegsChunkID);
    gen->builder.CreateStore(regOffset, resultRegister);
    gen->builder.CreateStore(prevFrame_chunkID, prevFrameChunkID);

    // Return the new frame
    return frame;
}

Value* ProcessStack::getPrevPCFromFrame(Value* frame) {
    frame = gen->builder.CreatePointerCast(frame, t_framep);
    auto prevPC = gen->builder.CreateGEP( frame
                                       , { ConstantInt::get(gen->t_int, 0)
                                         , ConstantInt::get(gen->t_int, 0)
                                         }
                                       );
    return gen->builder.CreateLoad(prevPC);
}

Value* ProcessStack::getPrevRegisterChunkIDFrame(Value* frame) {
    frame = gen->builder.CreatePointerCast(frame, t_framep);
    auto prevRegsChunkID = gen->builder.CreateGEP( frame
                                                , { ConstantInt::get(gen->t_int, 0)
                                                  , ConstantInt::get(gen->t_int, 1)
                                                  }
                                                );
    return gen->builder.CreateLoad(prevRegsChunkID);
}

Value* ProcessStack::getResultRegisterOffsetFromFrame(Value* frame) {
    frame = gen->builder.CreatePointerCast(frame, t_framep);
    auto resultRegister = gen->builder.CreateGEP( frame
                                               , { ConstantInt::get(gen->t_int, 0)
                                                 , ConstantInt::get(gen->t_int, 2)
                                                 }
                                               );
    return gen->builder.CreateLoad(resultRegister);
}

Value* ProcessStack::getPrevStackChunkIDFromFrame(Value* frame) {
    frame = gen->builder.CreatePointerCast(frame, t_framep);
    auto prevFrameChunkID = gen->builder.CreateGEP( frame
                                                 , { ConstantInt::get(gen->t_int, 0)
                                                   , ConstantInt::get(gen->t_int, 3)
                                                   }
                                                 );
    return gen->builder.CreateLoad(prevFrameChunkID);
}

void ProcessStack::pushStackFrame(GenerationContext* gctx, Function& F, std::vector<Value*> const& args, CallInst* callSite, Value* targetThreadID) {
    auto& builder = gen->builder;

    if(!targetThreadID) {
        targetThreadID = gctx->thread_id;
    }

    llvmgen::BBComment(builder, "pushStackFrame");

    auto dst_pc = gen->lts["processes"][targetThreadID]["pc"].getValue(gctx->svout);
    auto dst_reg = gen->lts["processes"][targetThreadID]["r"].getValue(gctx->svout);

    // If this is the setup call for main or a new thread
    if(callSite == nullptr) {

        // Merely push an empty stack
        auto pStackChunkID = gen->lts["processes"][targetThreadID]["stk"].getValue(gctx->svout);
        gen->builder.CreateStore(getEmptyStack(gctx), pStackChunkID);

    } else {

        // Create new register frame chunk containing the current register values
        ChunkMapper cm_rframe(gctx, gen->type_register_frame);
        Value* chunkid = cm_rframe.generatePut( gen->t_registers_max_size
                                              , dst_reg
                                              );

        // Create new frame
        auto pStackChunkID = gen->lts["processes"][targetThreadID]["stk"].getValue(gctx->svout);
        auto stackChunkID = builder.CreateLoad(pStackChunkID, "stackChunkID");
        auto frame = generateNewFrame( gen->builder.CreateLoad(dst_pc, "pc")
                                     , chunkid
                                     , callSite
                                     , stackChunkID
                                     );

        // Put the new frame on the stack
        ChunkMapper cm_stack(gctx, gen->type_stack);
        auto newStackChunkID = cm_stack.generatePut(gen->generateAlignedSizeOf(t_frame), frame);
        gctx->gen->setDebugLocation(newStackChunkID, __FILE__, __LINE__ - 1);
        gen->builder.CreateStore(newStackChunkID, pStackChunkID);

    }

    // Check if the number of arguments is correct
    // Too few arguments results in 0-values for the later parameters
    if(args.size() > F.arg_size()) {
        std::cout << "calling function with too many arguments" << std::endl;
        std::cout << "  #arguments: " << args.size() << std::endl;
        std::cout << "  #params: " << F.arg_size() << std::endl;
        assert(0);
    }

    // Assign the arguments to the parameters
    auto param = F.arg_begin();
    for(auto& arg: args) {

        // Map and load the argument
        auto v = gen->vMap(gctx, arg);

        // Map the parameter register to the location in the state vector
        auto vParam = gen->vReg(dst_reg, &*param);

        // Perform the store
        gen->builder.CreateStore(v, vParam);

        // Next
        param++;
    }

    // Set the program counter to the start of the pushed function
    auto loc = gen->programLocations[&*F.getEntryBlock().begin()];
    assert(loc);
    gen->builder.CreateStore(ConstantInt::get(gen->t_int, loc), dst_pc);
//    builder.CreateCall( gen->pins("printf")
//            , { gen->generateGlobalString("Call: jumping to %u\n")
//                                , ConstantInt::get(gen->t_int, loc)
//                        }
//    );
}

void ProcessStack::popStackFrame(GenerationContext* gctx, ReturnInst* result) {
    auto& builder = gen->builder;

    llvmgen::BBComment(builder, "popStackFrame");

    auto dst_pc = gen->lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);
    auto pStackChunkID = gen->lts["processes"][gctx->thread_id]["stk"].getValue(gctx->svout);
    auto registers = gen->lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
    auto stackChunkID = builder.CreateLoad(pStackChunkID, "stackChunkID");

    // Load the return value from the current registers
    Value* retVal = nullptr;
    if(result && result->getReturnValue()) {
        retVal = gen->vMap(gctx, result->getReturnValue());
    }

    // Generate an if for when this is the last frame or not
    llvmgen::If If(builder, "if_there_are_frames");
    If.setCond(builder.CreateICmpNE(stackChunkID, getEmptyStack(gctx)));
    BasicBlock* BBTrue = If.getTrue();
    BasicBlock* BBFalse = If.getFalse();
    If.generate();


    // When there is a frame to be popped
    {
        builder.SetInsertPoint(&*BBTrue->getFirstInsertionPt());

        // Get the stack
        ChunkMapper cm_stack(gctx, gen->type_stack);
        Value* chunk = cm_stack.generateGet(stackChunkID);
        Value* chData = gen->generateChunkGetData(chunk);

        // Pop the last frame from the stack by merely using the chunkID of
        // the precious stackframe
        builder.CreateStore(getPrevStackChunkIDFromFrame(chData), pStackChunkID);

        // Load the PC of the caller function from the frame and store that as the new PC
        auto prevPC = getPrevPCFromFrame(chData);
        gen->builder.CreateStore(prevPC, dst_pc);

        // Restore stored registers
        auto prevRegsChunkID = getPrevRegisterChunkIDFrame(chData);

        ChunkMapper cm_rframe(gctx, gen->type_register_frame);
        Value* chunkReg = cm_rframe.generateGet(prevRegsChunkID);
        Value* chRegLen = gen->generateChunkGetLen(chunkReg);
        Value* chRegData = gen->generateChunkGetData(chunkReg);

        gen->builder.CreateMemCpy( registers
                                 , registers->getPointerAlignment(gen->pinsModule->getDataLayout())
                                 , chRegData
                                 , chRegData->getPointerAlignment(gen->pinsModule->getDataLayout())
                                 , chRegLen
                                 );

        // Store the result in the right register, if there is a return value
        if(retVal) {
            auto offset = getResultRegisterOffsetFromFrame(chData);
//                    gen->builder.CreateCall( gen->pins("printf")
//                                          , { gen->generateGlobalString("Popping frame, result register offset %i, result %i\n")
//                                            , offset
//                                            , retVal
//                                            }
//                                          );
            auto resultRegister = gen->vRegUsingOffset(registers, offset, result->getReturnValue()->getType());
            gen->builder.CreateStore(retVal, resultRegister);
        }

    }

    // When there is no frame to be popped
    {

        // Set the PC of this thread to terminated
        builder.SetInsertPoint(&*BBFalse->getFirstInsertionPt());
        gen->builder.CreateStore(ConstantInt::get(gen->t_int, 0), dst_pc);

        // Reset registers to 0
        builder.CreateMemSet( registers
                , ConstantInt::get(gen->t_int8, 0)
                , gen->t_registers_max_size
                , registers->getPointerAlignment(gen->pinsModule->getDataLayout())
        );


        // If there is a return value, store the result in the list of thread
        // results, such that it can be obtained later by for example pthread_join()
        if(retVal) {

            // Create a new tuple {tid,result}
            auto t_tres = StructType::get(gen->ctx, {gen->t_int64, gen->t_voidp}, true);
            auto newTres = gen->builder.CreateAlloca(t_tres);

            auto newTres_tid = gen->builder.CreateGEP( t_tres
                                                     , newTres
                                                     , { ConstantInt::get(gen->t_int, 0)
                                                       , ConstantInt::get(gen->t_int, 0)
                                                       }
                                                     );
            auto newTres_res = gen->builder.CreateGEP( t_tres
                                                     , newTres
                                                     , { ConstantInt::get(gen->t_int, 0)
                                                       , ConstantInt::get(gen->t_int, 1)
                                                       }
                                                     );

            // Set the TID of the tuple
            auto tid_p = gen->lts["processes"][gctx->thread_id]["tid"].getValue(gctx->src);
            auto tid32 = gen->builder.CreateLoad(tid_p);
            auto tid = builder.CreateIntCast(tid32, gen->t_int64, false);
            gen->builder.CreateStore(tid, newTres_tid);

            // Set the result of the tuple
            Value* retValVoidP;
            if(retVal->getType()->isIntegerTy()) retValVoidP = builder.CreateIntToPtr(retVal, gen->t_voidp);
            else if(retVal->getType()->isPointerTy()) retValVoidP = builder.CreatePointerCast(retVal, gen->t_voidp);
            else retValVoidP = ConstantPointerNull::get(gen->t_voidp);

            gen->builder.CreateStore(retValVoidP, newTres_res);

//            builder.CreateCall( gen->pins("printf")
//                    , { gen->generateGlobalString("return a value %x %p\n")
//                      , tid
//                      , retValVoidP
//                      }
//                    );

            // Append the new tuple to the list of tuples
            // OLD STYLE: this is using appending, but we will do array-style setting instead using the ID as index
//            ChunkMapper cm_tres = ChunkMapper(gctx, gen->type_threadresults);
//            auto tres_p = gen->lts["tres"].getValue(gctx->svout);
//            auto newListOfTRes = cm_tres.generateCloneAndAppend(tres_p, newTres, gen->generateSizeOf(t_tres));
//            gen->builder.CreateStore(newListOfTRes, tres_p);

            ChunkMapper cm_tres = ChunkMapper(gctx, gen->type_threadresults);
            auto tres_p = gen->lts["tres"].getValue(gctx->svout);
            auto siz = gen->generateSizeOf(t_tres);
            auto offset = gen->builder.CreateMul(siz, tid32);
            auto newListOfTRes = cm_tres.generateCloneAndModify(tres_p, offset, newTres, siz);
            gen->builder.CreateStore(newListOfTRes, tres_p);

        }
    }

    // Continue the rest of the code after the If
    builder.SetInsertPoint(If.getFinal());

}

} // namespace llmc
