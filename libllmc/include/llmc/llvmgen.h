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
    IRBuilder<>& builder;
    Value* Cond;
    BasicBlock* BBTrue;
    BasicBlock* BBFalse;
    BasicBlock* BBFinal;
    bool usedBBTrue;
    bool usedBBFalse;
    BasicBlock* insertionPoint;
public:

    If(IRBuilder<>& builder, std::string const& name)
    : builder(builder)
    , Cond(nullptr)
    , BBTrue(nullptr)
    , BBFalse(nullptr)
    , BBFinal(nullptr)
    , usedBBTrue(false)
    , usedBBFalse(false)
    , insertionPoint(nullptr)
    {
        insertionPoint = builder.GetInsertBlock();
        BBTrue = BasicBlock::Create(insertionPoint->getContext(), name + "_true");
        BBFalse = BasicBlock::Create(insertionPoint->getContext(), name + "_false");
        BBFinal = BasicBlock::Create(insertionPoint->getContext(), name + "_final");
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

        assert(!insertionPoint->getTerminator());

        builder.SetInsertPoint(insertionPoint);

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
            BBTrue->insertInto(insertionPoint->getParent());
            if(!BBTrue->getTerminator()) {
                builder.SetInsertPoint(BBTrue);
                builder.CreateBr(BBFinal);
            }
        } else {
            delete BBTrue;
        }

        if(usedBBFalse) {
            BBFalse->insertInto(insertionPoint->getParent());
            if(!BBFalse->getTerminator()) {
                builder.SetInsertPoint(BBFalse);
                builder.CreateBr(BBFinal);
            }
        } else {
            delete BBFalse;
        }

        BBFinal->insertInto(insertionPoint->getParent());

        builder.SetInsertPoint(BBFinal);

    }

};

/**
 * @class If
 * @author Freark van der Berg
 * @date 04/07/17
 * @file llvmgen.h
 * @brief Helper class to create an if statement in LLVM
 */
class If2 {
private:
    IRBuilder<>& builder;
    Value* Cond;
    BasicBlock* BBTrue;
    BasicBlock* BBFalse;
    BasicBlock* BBFinal;
    bool usedBBTrue;
    bool usedBBFalse;
    BasicBlock* insertionPoint;
public:

    If2(IRBuilder<>& builder, Value* Cond, std::string const& name, BasicBlock* BBFinal)
    : builder(builder)
    , Cond(Cond)
    , BBTrue(nullptr)
    , BBFalse(nullptr)
    , BBFinal(BBFinal)
    {
        BBTrue = BasicBlock::Create(builder.getContext(), name + "_true", builder.GetInsertBlock()->getParent());
        BBFalse = BasicBlock::Create(builder.getContext(), name + "_false", builder.GetInsertBlock()->getParent());
        builder.CreateCondBr(Cond, BBTrue, BBFalse);
    }

    If2(IRBuilder<>& builder, Value* Cond, std::string const& name)
            : builder(builder)
            , Cond(Cond)
            , BBTrue(nullptr)
            , BBFalse(nullptr)
            , BBFinal(nullptr)
            , usedBBFalse(0)
            , usedBBTrue(0)
    {
        BBTrue = BasicBlock::Create(builder.getContext(), name + "_true", builder.GetInsertBlock()->getParent());
        BBFalse = BasicBlock::Create(builder.getContext(), name + "_false", builder.GetInsertBlock()->getParent());
        BBFinal = BasicBlock::Create(builder.getContext(), name + "_final", builder.GetInsertBlock()->getParent());
        builder.CreateCondBr(Cond, BBTrue, BBFalse);
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

    void startTrue() {
        usedBBTrue = true;
        builder.SetInsertPoint(BBTrue);
    }
    void startFalse() {
        usedBBFalse = true;
        builder.SetInsertPoint(BBFalse);
    }
    void endTrue() {
        builder.CreateBr(BBFinal);
    }
    void endFalse() {
        builder.CreateBr(BBFinal);
    }
    void finally() {
        if(!usedBBFalse) {
            builder.SetInsertPoint(BBFalse);
            builder.CreateBr(BBFinal);
        }
        if(!usedBBTrue) {
            builder.SetInsertPoint(BBTrue);
            builder.CreateBr(BBFinal);
        }
        builder.SetInsertPoint(BBFinal);
    }

};

class While {
private:
    IRBuilder<>& builder;
    Value* Cond;
    BasicBlock* BBBody;
    BasicBlock* BBFinal;
    Function* F;
    std::string name;
    Value* from;
    Value* to;
public:

    While(IRBuilder<>& builder, std::string const& name)
    : builder(builder)
    , Cond(nullptr)
    , BBBody(nullptr)
    , BBFinal(nullptr)
    , F(nullptr)
    , name(name)
    , from(nullptr)
    , to(nullptr)
    {
        F = builder.GetInsertBlock()->getParent();
        BBFinal = BasicBlock::Create(F->getContext(), name + "_final");
    }

    While& setCond(Value* Cond) {
        this->Cond = Cond;
        return *this;
    }
    While& setBody(BasicBlock* BB) {
        BBBody = BB;
        return *this;
    }
    While& setFinal(BasicBlock* BB) {
        BBFinal = BB;
        return *this;
    }

    Value*& getCond() {
        return Cond;
    }
    BasicBlock*& getBody() {
        if(!BBBody) BBBody = BasicBlock::Create(F->getContext(), name + "_true");
        return BBBody;
    }
    BasicBlock*& getFinal() {
        return BBFinal;
    }

    void generate() {

        if(BBBody) {
            BBBody->insertInto(F);
            builder.SetInsertPoint(BBBody);
            builder.CreateBr(BBFinal);
        } else {
            builder.CreateBr(BBFinal);
        }

        BBFinal->insertInto(F);

        builder.SetInsertPoint(BBFinal);

    }

};

class For {
private:
    IRBuilder<>& builder;
    Value* Cond;
    BasicBlock* BBCond;
    BasicBlock* BBBody;
    BasicBlock* BBFinal;
    Function* F;
    std::string name;
    Value* from;
    Value* to;
    Value* incr;
    PHINode* value;
public:

    For(IRBuilder<>& builder, std::string const& name)
    : builder(builder)
    , Cond(nullptr)
    , BBCond(nullptr)
    , BBBody(nullptr)
    , BBFinal(nullptr)
    , F(nullptr)
    , name(name)
    , from(nullptr)
    , to(nullptr)
    , incr(nullptr)
    , value(nullptr)
    {
        F = builder.GetInsertBlock()->getParent();
        BBFinal = BasicBlock::Create(F->getContext(), name + "_final");
        BBCond = BasicBlock::Create(F->getContext(), name + "_cond");
    }

    For& setRange(Value* from, Value* to, Value* incr) {
        assert(from->getType() == to->getType());
        value = PHINode::Create(from->getType(), 2, "loop_value");
//        Cond = builder.CreateICmpULT(i, to));
//        i->addIncoming(from, builder.GetInsertBlock());
//        i->addIncoming(nextTG, BBBody);
        return *this;
    }

    For& setBody(BasicBlock* BB) {
        BBBody = BB;
        return *this;
    }
    For& setFinal(BasicBlock* BB) {
        BBFinal = BB;
        return *this;
    }

    Value*& getCond() {
        return Cond;
    }
    BasicBlock*& getBody() {
        if(!BBBody) BBBody = BasicBlock::Create(F->getContext(), name + "_true");
        return BBBody;
    }
    BasicBlock*& getCondBB() {
        if(!BBCond) BBCond = BasicBlock::Create(F->getContext(), name + "_true");
        return BBCond;
    }
    BasicBlock*& getFinal() {
        return BBFinal;
    }

    void generate() {

        if(BBBody) {

            builder.CreateCondBr(Cond, BBBody, BBFinal);
            BBBody->insertInto(F);
            builder.SetInsertPoint(BBBody);
            builder.CreateBr(BBFinal);
        } else {
            builder.CreateBr(BBFinal);
        }

        BBFinal->insertInto(F);

        builder.SetInsertPoint(BBFinal);

    }

};

/**
 * @class IRStringBuilder
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief Allows one to build an LLVM string using stream operators.
 *   IRStringBuilder irs;
 *   irs << someString;
 *   irs << someInt;
 *   builder.CreateCall(pins("puts"), {irs.str()});
 */
template<typename LLDMCModelGenerator>
class IRStringBuilder {
public:

    IRStringBuilder(LLDMCModelGenerator& gen, size_t max_chars)
    : gen(gen)
    , written(ConstantInt::get(gen.pins("snprintf")->getFunctionType()->getParamType(1), 0))
    , max_chars(max_chars)
    , max(ConstantInt::get(gen.pins("snprintf")->getFunctionType()->getParamType(1), max_chars))
    {
        out = gen.addAlloca(ArrayType::get(gen.t_int8, max_chars), gen.builder.GetInsertBlock()->getParent());
    }

    IRStringBuilder& operator<<(long long int i) {
        Value* write_at = gen.builder.CreateGEP(out, {ConstantInt::get(gen.t_int, 0), written});
        Value* remaining = gen.builder.CreateSub(max, written);
        Value* written_here = gen.builder.CreateCall( gen.pins("snprintf")
                                                    , {write_at
                                                      , remaining
                                                      , gen.generateGlobalString("%i")
                                                      , ConstantInt::get(gen.t_int, i)
                                                      }
                                                    );
        auto wh = gen.builder.CreateIntCast( written_here
                                           , gen.pins("snprintf")->getFunctionType()->getParamType(1)
                                           , false
                                           );
        written = gen.builder.CreateAdd(written, wh);
        return *this;
    }
    IRStringBuilder& operator<<(const char* c) {
        *this << std::string(c);
        return *this;
    }
    IRStringBuilder& operator<<(std::string s) {
        Value* write_at = gen.builder.CreateGEP(out, {ConstantInt::get(gen.t_int, 0), written});
        Value* remaining = gen.builder.CreateSub(max, written);
        Value* written_here = gen.builder.CreateCall( gen.pins("snprintf")
                                                    , { write_at
                                                      , remaining
                                                      , gen.generateGlobalString("%s")
                                                      , gen.generateGlobalString(s)
                                                      }
                                                    );
        auto wh = gen.builder.CreateIntCast( written_here
                                           , gen.pins("snprintf")->getFunctionType()->getParamType(1)
                                           , false
                                           );
        written = gen.builder.CreateAdd(written, wh);
        return *this;
    }

    IRStringBuilder& operator<<(Value* value) {
        Value* write_at = gen.builder.CreateGEP(out, {ConstantInt::get(gen.t_int, 0), written});
        Value* remaining = gen.builder.CreateSub(max, written);
        Value* written_here = nullptr;
        if(value->getType()->isIntegerTy()) {
            value = gen.builder.CreateIntCast(value, gen.t_int, true);
            written_here = gen.builder.CreateCall(gen.pins("snprintf")
                                                 , { write_at
                                                   , remaining
                                                   , gen.generateGlobalString("%i")
                                                   , value
                                                   }
                                                );
        } else {
            assert(0 && "non supported value for printing");
        }
        auto wh = gen.builder.CreateIntCast( written_here
                                           , gen.pins("snprintf")->getFunctionType()->getParamType(1)
                                           , false
                                           );
        written = gen.builder.CreateAdd(written, wh);
        return *this;
    }

    Value* str() {
        return gen.builder.CreateGEP( out
                                    , { ConstantInt::get(gen.t_int,0,true)
                                      , ConstantInt::get(gen.t_int,0,true)
                                      }
                                    );
    }

private:
    LLDMCModelGenerator& gen;
    Value* written;
    size_t max_chars;
    Value* max;
    Value* out;
};

/**
 * @brief Places a branch to a new BasicBlock as a comment
 * @param comment The name of the BB that serves as a comment
 */
void BBComment(IRBuilder<>& builder, std::string const& comment);

} // namespace llvmgen
