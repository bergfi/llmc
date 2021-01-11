#pragma once

#include <llmc/generation/GenerationContext.h>

namespace llmc {

/**
 * @class ProcessStack
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Class for helper functions for the stack of a process
 */
class ProcessStack {
private:
    LLPinsGenerator* gen;
//        void pushRegisters(GenerationContext* gctx) {
//            auto& gen = *gctx->gen;
//        }
//
//        void popRegisters(GenerationContext* gctx) {
//            auto& gen = *gctx->gen;
//            auto stackChunkID = gen.lts["processes"][gctx->thread_id]["stk"].getLoadedValue(gctx->svout);
//            auto stackHasRegisters = gen.builder.CreateICmpNE(stackChunkID, ConstantInt::get(gen.t_int, 0));
//        }

    /**
     * Type of a single frame
     */
    StructType* t_frame;

    /**
     * Type of a pointer to a frame
     */
    PointerType* t_framep;

    /**
     * ChunkID of an empty stack, set when model is loaded
     */
    GlobalVariable* GV_emptyStack;

public:
    ProcessStack(LLPinsGenerator* gen)
    : gen(gen)
    , GV_emptyStack(nullptr)
    {
    }

    void init();

    /**
     * @brief Returns the chunkID of an empty stack
     * Creates a GLOBAL variable and generates a pins_chunk_put with an empty stack
     * This is done ONCE and on subsequent calls, the cached global is returned.
     * @param model The LTSmin model
     * @return The chunkID of an empty stack
     */
    Value* getEmptyStack(GenerationContext* gctx);

    /**
     * @brief Generates a new stackframe
     * The parameter @c callSite is used to determine where in the register
     * the result should be stored.
     * @param oldPC The PC to jump back to when this frame is popped
     * @param rframe_chunkID The chunkID of the registers to retore when this frame is popped
     * @param callSite The call instruction that causes this new frame
     * @param prevFrame_chunkID The chunkID of the previous stackframe on the stack
     * @return Pointer to the memory location of the new frame
     */
    Value* generateNewFrame(Value* oldPC, Value* rframe_chunkID, CallInst* callSite, Value* prevFrame_chunkID);

    Value* getPrevPCFromFrame(Value* frame);

    Value* getPrevRegisterChunkIDFrame(Value* frame);

    Value* getResultRegisterOffsetFromFrame(Value* frame);

    Value* getPrevStackChunkIDFromFrame(Value* frame);

    /**
     * @brief Push a new frame on the stack, describing the current
     * call-state and how to revert to it. Then sets the program
     * counter to the start of the specified function.
     * This is used for example when calling a function to save e.g.
     * the registers and the program counter.
     * To revert to the previous frame, use @c popStackFrame().
     *
     * Changes:
     *   - ["processes"][gctx->thread_id]["pc"]
     *   - ["processes"][gctx->thread_id]["stk"]
     *
     * @param gctx GenerationContext
     * @param F The function
     * @param setupCall
     */
    void pushStackFrame(GenerationContext* gctx, Function& F, std::vector<Value*> const& args, CallInst* callSite, Value* targetThreadID = nullptr);

    /**
     * @brief Pops the latest frame from the stack and restores e.g.
     * registers and the program counter to those specified by that
     * frame.
     * This used used for example by the handling of the Ret instruction.
     *
     * Changes:
     *   - ["processes"][gctx->thread_id]["pc"]
     *   - ["processes"][gctx->thread_id]["stk"]
     *   - ["processes"][gctx->thread_id]["r"]
     *
     * @param gctx GenerationContext
     * @param result The return instruction that causes the pop
     */
    void popStackFrame(GenerationContext* gctx, ReturnInst* result);

};

} // namespace llmc
