#pragma once

#include "config.h"

#include "llvm/IR/IRBuilder.h"

namespace llvmgen {

using namespace llvm;

/**
 * @class If
 * @author Freark van der Berg
 * @date 04/07/17
 * @file llvmgen.h
 * @brief Helper class to create an if statement in LLVM
 */
class If {
private:
    using BUILDER = IRBuilder<>;
    BUILDER& builder;
    Value* Cond;
    BasicBlock* BBTrue;
    BasicBlock* BBFalse;
    BasicBlock* BBFinal;
    bool usedBBTrue;
    bool usedBBFalse;
    Function* F;
public:

    If(BUILDER& builder, std::string const& name)
    : builder(builder)
    {
        F = builder.GetInsertBlock()->getParent();
        BBTrue = BasicBlock::Create(F->getContext(), name + "_true");
        BBFalse = BasicBlock::Create(F->getContext(), name + "_false");
        BBFinal = BasicBlock::Create(F->getContext(), name + "_final");
    }

    If& setCond(Value* Cond) {
        this->Cond = Cond;
        return *this;
    }
    If& setTrue(BasicBlock* BB) {
        BBTrue = BB;
        return *this;
    }
    If& setFalse(BasicBlock* BB) {
        BBFalse = BB;
        return *this;
    }
    If& setFinal(BasicBlock* BB) {
        BBFinal = BB;
        return *this;
    }

    Value*& getCond() {
        this->Cond = Cond;
        return Cond;
    }
    BasicBlock*& getTrue() {
        usedBBTrue = true;
        return BBTrue;
    }
    BasicBlock*& getFalse() {
        usedBBFalse = true;
        return BBFalse;
    }
    BasicBlock*& getFinal() {
        return BBFinal;
    }

    void generate() {

        if(usedBBFalse) {
            if(usedBBTrue) {
                builder.CreateCondBr(Cond, BBTrue, BBFalse);
            } else {
                builder.CreateCondBr(Cond, BBFinal, BBFalse);
            }
        } else {
            if(usedBBTrue) {
                builder.CreateCondBr(Cond, BBTrue, BBFinal);
            } else {
                builder.CreateBr(BBFinal);
            }
        }

        if(usedBBTrue) {
            BBTrue->insertInto(F);
            builder.SetInsertPoint(BBTrue);
            builder.CreateBr(BBFinal);
        } else {
            delete BBTrue;
        }

        if(usedBBFalse) {
            BBFalse->insertInto(F);
            builder.SetInsertPoint(BBFalse);
            builder.CreateBr(BBFinal);
        } else {
            delete BBFalse;
        }

        BBFinal->insertInto(F);

        builder.SetInsertPoint(BBFinal);

    }

};

/**
 * @brief Places a branch to a new BasicBlock as a comment
 * @param comment The name of the BB that serves as a comment
 */
void BBComment(IRBuilder<>& builder, std::string const& comment);

} // namespace llvmgen
