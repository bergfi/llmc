#pragma once

namespace llmc {

/**
 * @class TransitionGroup
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief Describes a single transition group.
 * Serves as a base class for @c TransitionGroupInstruction
 * and @c TransitionGroupLambda.
 */
class TransitionGroup {
public:

    virtual ~TransitionGroup() {

    }

    enum class Type {
        Instructions,
        Lambda
    };

    TransitionGroup()
    : nextState(nullptr)
    {
    }


    Type getType() const {
        return type;
    }

    void setDesc(std::string s) {
        desc = std::move(s);
    }

protected:
    Type type;
private:
    BasicBlock* nextState;
    std::string desc;
    friend class LLDMCModelGenerator;
};

/**
 * @class TransitionGroupInstruction
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief Describes the transition of a single instruction.
 */
class TransitionGroupInstructions: public TransitionGroup {
public:
    TransitionGroupInstructions( int thread_id
                               , std::vector<Instruction*> instructions
                               , std::vector<llvm::Value*> conditions
                               , std::vector<Instruction*> actions
                               , bool emitter
                               )
    : thread_id(thread_id)
    , instructions(instructions)
    , conditions(std::move(conditions))
    , actions(std::move(actions))
    , _emitter(emitter)
    {
        type = TransitionGroup::Type::Instructions;
    }
private:

    int thread_id;

    /**
     * The instruction that this transition groups describes. It serves both
     * as the instruction that is executed and the instruction of the source
     * program counter.
     */
    std::vector<Instruction*> instructions;

    /**
     * Extra conditions of type i1 that need to hold.
     */
    std::vector<llvm::Value*> conditions;

    /**
     * Extra instructions that are executed AFTER srcPC.
     */
    std::vector<Instruction*> actions;

    bool _emitter;

    friend class LLDMCModelGenerator;
};

/**
 * @class TransitionGroupLambda
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief A transition group that is described solely by the lambda inside
 */
class TransitionGroupLambda: public TransitionGroup {
public:
    template<typename T>
    TransitionGroupLambda(T&& lambda)
    {
        type = TransitionGroup::Type::Lambda;
        f = lambda;
    }

    void execute(GenerationContext* gctx, BasicBlock* noReportBB) {
        f(gctx, this, noReportBB);
    }

private:
    std::function<void(GenerationContext*, TransitionGroupLambda*, BasicBlock*)> f;
    friend class LLDMCModelGenerator;
};

} // namespace llmc
