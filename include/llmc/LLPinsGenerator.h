#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stack>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <llmc/llvmincludes.h>

#include <libfrugi/MessageFormatter.h>
#include <libfrugi/FileSystem.h>
#include <libfrugi/System.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>

#include "llvmgen.h"

extern "C" {
#include "popt.h"
}

using namespace llvm;

namespace llmc {

/**
 * @class TransitionGroup
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Describes a single transition group.
 * Serves as a base class for @c TransitionGroupInstruction
 * and @c TransitionGroupLambda.
 */
class TransitionGroup {
public:
    enum class Type {
        Instruction,
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
    friend class LLPinsGenerator;
};

/**
 * @class TransitionGroupInstruction
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Describes the transition of a single instruction.
 */
class TransitionGroupInstruction: public TransitionGroup {
public:
    TransitionGroupInstruction( Instruction* srcPC
                              , Instruction* dstPC
                              , std::vector<llvm::Value*> conditions
                              , std::vector<Instruction*> actions
                              )
    : srcPC(srcPC)
    , dstPC(dstPC)
    , conditions(std::move(conditions))
    , actions(std::move(actions))
    {
        type = TransitionGroup::Type::Instruction;
    }
private:

    /**
     * The instruction that this transition groups describes. It serves both
     * as the instruction that is executed and the instruction of the source
     * program counter.
     */
    Instruction* srcPC;

    /**
     * The instruction that describes the destination program counter. In other
     * words: after this transition, the PC should be set to dstPC. Even in the
     * case of a call instruction, because this is the PC to return to.
     */
    Instruction* dstPC;

    /**
     * Extra conditions of type i1 that need to hold.
     */
    std::vector<llvm::Value*> conditions;

    /**
     * Extra instructions that are executed AFTER srcPC.
     */
    std::vector<Instruction*> actions;

    friend class LLPinsGenerator;
};

class LLPinsGenerator;

/**
 * @class GenerationContext
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Describes the current context while generating LLVM IR.
 * It remembers various details through the generation of a single transition
 * group.
 */
class GenerationContext {
public:

    /**
     * The generator instance
     */
    LLPinsGenerator* gen;

    /**
     * The LTSmin model
     */
    Value* model;

    /**
     * Pointer to the read-only state-vector of the incoming state
     */
    Value* src;

    /**
     * Pointer to the state-vector that will be reported to LTSmin
     */
    Value* svout;

    /**
     * Pointer to the edgle label array that will be reported to LTSmin
     */
    Value* edgeLabelValue;

    /**
     * Map from an edge label to the value of an edge label
     * Use this to specify the edge label of a transition.
     */
    std::unordered_map<std::string, Value*> edgeLabels;

    /**
     * The current thread_id
     */
    int thread_id;

    /**
     * Whether the transition group changed the PC
     */
    bool alteredPC;

    GenerationContext()
    :   gen(nullptr)
    ,   model(nullptr)
    ,   src(nullptr)
    ,   svout(nullptr)
    ,   edgeLabelValue(nullptr)
    ,   edgeLabels()
    ,   thread_id(0)
    ,   alteredPC(false)
    {
    }
};

/**
 * @class TransitionGroupLambda
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
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

    void execute(GenerationContext* gctx) {
        f(gctx, this);
    }

private:
    std::function<void(GenerationContext*, TransitionGroupLambda*)> f;
    friend class LLPinsGenerator;
};

/**
 * @class FunctionData
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Holds information about a single function body.
 */
struct FunctionData {

    /**
     * The LLVM Type describing the registers used
     */
    Type* registerLayout;

    /**
     * Contains the types of the all registers
     */
    std::vector<Type*> registerTypes;

    FunctionData()
    : registerLayout(nullptr)
    {
    }
};

/**
 * @class LLPinsGenerator
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief The class that does the work
 */
class LLPinsGenerator {
public:

    friend class ChunkMapper;
    friend class ProcessStack;

    /**
     * @class IRStringBuilder
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Allows one to build an LLVM string using stream operators.
     *   IRStringBuilder irs;
     *   irs << someString;
     *   irs << someInt;
     *   builder.CreateCall(pins("puts"), {irs.str()});
     */
    class IRStringBuilder {
    public:

        IRStringBuilder(LLPinsGenerator& gen, size_t max_chars)
        : gen(gen)
        , written(ConstantInt::get(gen.pins("snprintf")->getFunctionType()->getParamType(1), 0))
        , max_chars(max_chars)
        , max(ConstantInt::get(gen.pins("snprintf")->getFunctionType()->getParamType(1), max_chars))
        {
            out = addAlloca(ArrayType::get(gen.t_int8, max_chars), gen.builder.GetInsertBlock()->getParent());
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
        LLPinsGenerator& gen;
        Value* written;
        size_t max_chars;
        Value* max;
        Value* out;
    };

    /**
     * @class SVType
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief A type in the tree of SVTree nodes.
     * @see SVTree
     */
    class SVType {
    public:
        SVType(std::string name, data_format_t ltsminType, Type* llvmType, LLPinsGenerator* gen)
        : _name(std::move(name))
        , _ltsminType(std::move(ltsminType))
        , _llvmType(llvmType)
        {

            // Determine if the LLVM Type fits perfectly in a number of state
            // vector slots.
            auto& DL = gen->pinsModule->getDataLayout();
            assert(_llvmType->isSized() && "type is not sized");
            auto bytesNeeded = DL.getTypeSizeInBits(_llvmType) / 8;
            assert(bytesNeeded > 0 && "size is 0");

            // If it does not, enlarge the type so that it does, but remember
            // the actual type that this SVType should represent
            if(bytesNeeded & 0x3) {
                _PaddedLLVMType = ArrayType::get( IntegerType::get(_llvmType->getContext(), 32)
                                                , (bytesNeeded + 3) / 4
                                                );
            } else {
                _PaddedLLVMType = _llvmType;
            }
        }

        /**
         * @brief Request the LLVMType of this SVType.
         * @return The LLVM Type of this SVType
         */
        Type* getLLVMType() {
            return _PaddedLLVMType;
        }

        /**
         * @brief Request the real LLVMType of this SVType.
         * The different with @c getLLVMType() is that in the event the type
         * does not perfectly fit a number of state slots, the type in the
         * SV can differ from the real type, because padding is added to
         * make it fit.
         * @return The real LLVM Type of this SVType
         */
        Type* getRealLLVMType() {
            return _llvmType;
        }
    public:
        std::string _name;
        data_format_t _ltsminType;
        int index;
    private:
        Type* _llvmType;
        Type* _PaddedLLVMType;
    };

    /**
     * @class SVTree
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief The State-Vector Tree nodes are the way to describe a state-vector.
     * Use this to describe the layout of a flattened tree-like structure and
     * gain easy access methods to the data.
     */
    class SVTree {
    protected:

        SVTree( LLPinsGenerator* gen
              , std::string name = ""
              , SVType* type = nullptr
              , SVTree* parent = nullptr
              , int index = 0
              )
        :   _gen(gen)
        ,   name(std::move(name))
        ,   typeName(std::move(typeName))
        ,   parent(std::move(parent))
        ,   index(std::move(index))
        ,   type(type)
        ,   llvmType(nullptr)
        {
        }

        LLPinsGenerator* gen() {
            SVTree* p = parent;
            while(!_gen && p) {
                _gen = p->_gen;
                p = p->parent;
            }
            return _gen;
        }

    public:

        SVTree() {
            assert(0);
        }

        /**
         * @brief Creates a new SVTree node with the specified type.
         * This should be used for leaves.
         * @todo possibly create SVLeaf type
         * @param name The name of the state-vector variable
         * @param type The type of the state-vector variable
         * @param parent The parent SVTree node
         * @return
         */
        SVTree(std::string name, SVType* type = nullptr, SVTree* parent = nullptr)
        :   _gen(nullptr)
        ,   name(std::move(name))
        ,   typeName(std::move(typeName))
        ,   parent(std::move(parent))
        ,   index(0)
        ,   type(type)
        ,   llvmType(nullptr)
        {
        }

        /**
         * @brief Creates a new SVTree node with the specified type.
         * This should be used for non-leaves.
         * @todo possibly create SVLeaf type
         * @param name The name of the state-vector variable
         * @param type The name to give the LLVM type that is the result of the
         *             list of types of the children of this node
         * @param parent The parent SVTree node
         * @return
         */
        SVTree(std::string name, std::string typeName, SVType* type = nullptr, SVTree* parent = nullptr)
        :   _gen(nullptr)
        ,   name(std::move(name))
        ,   typeName(std::move(typeName))
        ,   parent(std::move(parent))
        ,   index(0)
        ,   type(type)
        ,   llvmType(nullptr)
        {
        }

        /**
         * @brief Deletes this node and its children.
         */
        ~SVTree() {
            for(auto& c: children) {
                delete c;
            }
        }

        /**
         * @brief Accesses the child with the specified name
         * @param name The name of the child to access.
         * @return The SVTree child node with the specified name.
         */
        SVTree& operator[](std::string name) {
            for(auto& c: children) {
                if(c->getName() == name) {
                    return *c;
                }
            }
            assert(0);
            auto i = children.size();
            children.push_back(new SVTree(gen(), name, nullptr, this, i));
            return *children[i];
        }

        /**
         * @brief Accesses the child at the specified index
         * @param i The index of the child to access.
         * @return The SVTree child node at the specified index.
         */
        SVTree& operator[](size_t i) {
            if(i >= children.size()) {
                assert(0);
                children.resize(i+1);
                children[i] = new SVTree(gen(), name, nullptr, this, i);
            }
            return *children[i];
        }

        /**
         * @brief Adds a copy of the node @c t as child
         * @param t The node to copy and add.
         * @return A reference to this node
         */
        SVTree& operator<<(SVTree t) {
            *this << new SVTree(std::move(t));
            return *this;
        }

        /**
         * @brief Adds the node @c t as child
         * Claims the ownership over @c t.
         * @param t The node to add.
         * @return A reference to this node
         */
        SVTree& operator<<(SVTree* t) {
            for(auto& c: children) {
                if(c->getName() == name) {
                    std::cout << "Duplicate member: " << name << std::endl;
                    assert(0);
                    return *this;
                }
            }
            auto i = children.size();
            t->index = i;
            t->parent = this;
            children.push_back(t);
            return *this;
        }

        std::string const& getName() const {
            return name;
        }

        std::string getFullName() {
            if(parent) {
                std::string pname = parent->getFullName();
                if(name == "") {
                    std::stringstream ss;
                    ss << pname << "[" << getIndex() << "]";
                    return ss.str();
                } else if(pname == "") {
                    return name;
                } else {
                    return pname + "_" + name;
                }
            } else {
                return "";
            }
        }

        std::string const& getTypeName() const {
            return name;
        }

        /**
         * @brief Returns the index at which this node is at in the list
         * of nodes of the parent node.
         * @return The index of this node.
         */
        int getIndex() const { return index; }
        SVTree* getParent() const { return parent; }

        /**
         * @brief Returns a pointer to the location of this node in the
         * state-vector, based on the specified @c rootValue
         * @param rootValue The root Value of the state-vector
         * @return A pointer to the location of this node in the state-vector
         */
        Value* getValue(Value* rootValue) {
            std::vector<Value*> indices;
            std::string name = "p";
            SVTree* p = this;

            // Collect all indices to know how to access this variable from
            // the outer structure
            while(p) {
                indices.push_back(ConstantInt::get(gen()->t_int, p->index));
                name = p->name + "_" + name;
                p = p->parent;
            }

            // Reverse the list so it goes from outer to inner
            std::reverse(indices.begin(), indices.end());

            // Assert we have a parent or our index is 0
            assert(parent || index == 0);

            // Create the GEP
            auto v = gen()->builder.CreateGEP(rootValue, ArrayRef<Value*>(indices));

            // If the real LLVM Type is different from the LLVM Type in the SV,
            // perform a pointer cast, so that the generated code actually uses
            // the real LLVM Type it expects instead of the LLVM Type in the SV.
            // These can differ in the event the type does not fit a number of
            // slots perfectly, thus padding is required.
            if(type && type->getLLVMType() != type->getRealLLVMType()) {
                v = gen()->builder.CreatePointerCast(v, PointerType::get(type->getRealLLVMType(), 0), name);
            } else {
                v->setName(name);
            }
            return v;
        }

        Value* getLoadedValue(Value* rootValue) {
            std::vector<Value*> indices;
            std::string name = "L";
            SVTree* p = this;

            // Collect all indices to know how to access this variable from
            // the outer structure
            while(p) {
                indices.push_back(ConstantInt::get(gen()->t_int, p->index));
                name = p->name + "_" + name;
                p = p->parent;
            }

            // Reverse the list so it goes from outer to inner
            std::reverse(indices.begin(), indices.end());

            // Assert we have a parent or our index is 0
            assert(parent || index == 0);

            return gen()->builder.CreateLoad( gen()->builder.CreateGEP( rootValue
                                                                      , ArrayRef<Value*>(indices)
                                                                      )
                                            , name
                                            );
        }

        /**
         * @brief Returns the number of bytes the variable described by this
         * SVTree node needs in the state-vector.
         * @return The number of bytes needed for the variable.
         */
        Constant* getSizeValue() {
            return gen()->generateSizeOf(getLLVMType());
        }

        /**
         * @brief Returns the number of state-vector slots the variable
         * described by this SVTree node needs in the state-vector.
         * @return The number of state-vector slots needed for the variable.
         */
        Constant* getSizeValueInSlots() {
            return ConstantExpr::getUDiv(getSizeValue(), ConstantInt::get(gen()->t_int, 4, true));
        }

        SVType* getType() {
            return type;
        }

        Type* getLLVMType() {
            if(type && type->getLLVMType()) {
                return type->getLLVMType();
            } else if(llvmType) {
                return llvmType;
            } else {
                std::vector<Type*> types;
                for(auto& c: children) {
                    types.push_back(c->getLLVMType());
                }
                auto type_name = typeName;
                if(!parent) type_name += "_root";
                llvmType = StructType::create(gen()->ctx, types, type_name);
                return llvmType;
            }
        }

        bool isEmpty() {
            return children.size() == 0 && !type;
        }

        bool verify() {
            bool r = true;
            for(int i = 0; (size_t)i < children.size(); ++i) {
                auto& c = children[i];
                assert(c->getIndex() == i);
                assert(c->getParent() == this);
                if(!c->verify()) r = false;
            }
            if(name.empty()) {
                gen()->out.reportFailure("Node without a name");
                gen()->out.indent();
                if(getType()) {
                    gen()->out.reportNote("Type: " + getType()->_name);
                }
                if(!typeName.empty()) {
                    gen()->out.reportNote("TypeName: " + typeName);
                }
                gen()->out.outdent();
            } else if(!getType()) {
                gen()->out.reportFailure("Type is not set: " + name);
                //std::cout <<  << name << std::endl;
                //assert(0);
                r = false;
            }
            assert(gen());
            return r;
        }

        /**
         * @brief Executes @c f for every leaf in the tree.
         */
        template<typename FUNCTION>
        void executeLeaves(FUNCTION &&f) {
            for(auto& c: children) {
                c->executeLeaves(std::forward<FUNCTION>(f));
            }
            if(children.size() == 0) {
                f(this);
            }
        }

        size_t count() const {
            return children.size();
        }

        std::vector<SVTree*>& getChildren() {
            return children;
        }

    protected:
        LLPinsGenerator* _gen;
        std::string name;
        std::string typeName;
        SVTree* parent;
        int index;

        SVType* type;
        Type* llvmType;
        std::vector<SVTree*> children;

        friend class LLVMLTS;

    };

    /**
     * @class VarRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief An SVTree node describing the root of a tree.
     */
    class VarRoot: public SVTree {
    public:
        VarRoot(LLPinsGenerator* gen, std::string name)
        :   SVTree(std::move(gen), name)
        {
        }
    };

    /**
     * @class SVRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief A root node describing a state-vector
     */
    class SVRoot: public VarRoot {
        using VarRoot::VarRoot;
    };

    /**
     * @class SVRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief A root node describing a label array
     */
    class SVLabelRoot: public VarRoot {
        using VarRoot::VarRoot;
    };

    /**
     * @class SVRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief A root node describing an edge label array
     */
    class SVEdgeLabelRoot: public VarRoot {
        using VarRoot::VarRoot;
    };

    /**
     * @class SVRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief An SVTree node describing a label
     */
    class SVLabel: public SVTree {
    };

    /**
     * @class SVRoot
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief An SVTree node describing an edge label
     */
    class SVEdgeLabel: public SVTree {
    };

    /**
     * @class ChunkMapper
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Maps chunkIDs to chunks.
     * Allows easy acces to the chunk system of LTSmin. Construction does not
     * cost anything at runtime.
     */
    class ChunkMapper {
    private:
        LLPinsGenerator& gen;
        Value* model;
        SVType* type;
        int idx;

        void init() {
            int index = 0;
            for(auto& t: gen.lts.getTypes()) {
                if(t->_name == type->_name) {
                    idx = index;
                    break;
                }
                index++;
            }
        }
    public:

        /**
         * @brief Constructs a chunk mapper and initializes it.
         * This does not cost any runtime performance.
         * @param gen LLPinsGenerator
         * @param model The LTSmin model
         * @param type The type to use for the chunk mapping.
         */
        ChunkMapper(LLPinsGenerator& gen, Value* model, SVType* type)
        : gen(gen)
        , model(model)
        , type(type)
        , idx(-1)
        {
            assert(type->_ltsminType == LTStypeChunk);
            init();
        }

        /**
         * @brief Returns a read-only chunk reference.
         * @param chunkid The chunkID of the chunk to return
         * @return Read-only chunk reference
         */
        Value* generateGet(Value* chunkid) {
            if(chunkid->getType()->isPointerTy()) {
                chunkid = gen.builder.CreateLoad(chunkid);
            }
            if(!chunkid->getType()->isIntegerTy()) {

                // Undefined reference since LLVM 5
                if(auto* I = dyn_cast<Instruction>(chunkid)) {
                    //I->getParent()->dump();
                } else {
                    //chunkid->dump();
                }
                assert(0);
            }
            return gen.builder.CreateCall( gen.pins("pins_chunk_get")
                                         , {model, ConstantInt::get(gen.t_int, idx), chunkid}
                                         , "chunk." + type->_name
                                         );
        }

        /**
         * @brief Generates the LLVM IR to upload a chunk to LTSmin.
         * @param chunk The chunk to upload
         * @return The chunkID now associated with the uploaded chunk
         */
        Value* generatePut(Value* chunk) {
            if(chunk->getType()->isPointerTy()) {
                chunk = gen.builder.CreateLoad(chunk);
                assert(0 && "this should not work");
            }
            assert(chunk->getType() == gen.t_chunk);
            return gen.builder.CreateCall( gen.pins("pins_chunk_put")
                                         , {model, ConstantInt::get(gen.t_int, idx), chunk}
                                         , "chunkid." + type->_name
                                         );
        }

        /**
         * @brief Generates the LLVM IR to upload a chunk to LTSmin.
         * @param len The length of the chunk to upload
         * @param data The data of the chunk to upload
         * @return The chunkID now associated with the uploaded chunk
         */
        Value* generatePut(Value* len, Value* data) {
            len = gen.builder.CreateIntCast(len, gen.t_chunk->getElementType(0), false);
            data = gen.builder.CreatePointerCast(data, gen.t_chunk->getElementType(1));
            Value* chunk = gen.builder.CreateInsertValue(UndefValue::get(gen.t_chunk), len, {0});
            chunk = gen.builder.CreateInsertValue(chunk, data, {1});
            gen.builder.CreateCall( gen.pins("printf")
                                  , {gen.generateGlobalString("CHUNK[" + type->_name + "] put of %i bytes:")
                                    , len
                                    }
                                  );
            gen.builder.CreateCall(gen.pins("llmc_print_chunk"), {data, len});
            gen.builder.CreateCall(gen.pins("printf"), {gen.generateGlobalString("\n")});
            return gen.builder.CreateCall( gen.pins("pins_chunk_put")
                                         , {model, ConstantInt::get(gen.t_int, idx), chunk}
                                         , "chunkid." + type->_name
                                         );
        }

        /**
         * @brief Generates the LLVM IR to upload a chunk to LTSmin.
         * @param len The length of the chunk to upload
         * @param data The data of the chunk to upload
         * @param chunkID the chunkID to associate to the specified chunk
         *
         * NOTE: this can only be used at initialization time, NOT
         * at model-checking time.
         *
         * @todo make check for that
         *
         * @return The chunkID associated with the uploaded chunk
         */
        Value* generatePutAt(Value* len, Value* data, int chunkID) {
            assert(isa<IntegerType>(len->getType()));
            assert(isa<PointerType>(data->getType()));
            len = gen.builder.CreateIntCast(len, gen.t_chunk->getElementType(0), false);
            data = gen.builder.CreatePointerCast(data, gen.t_chunk->getElementType(1));
            Value* chunk = gen.builder.CreateInsertValue(UndefValue::get(gen.t_chunk), len, {0});
            chunk = gen.builder.CreateInsertValue(chunk, data, {1});
            gen.builder.CreateCall( gen.pins("printf")
                                  , { gen.generateGlobalString("CHUNK[" + type->_name + "] put of %i bytes:")
                                    , len
                                    }
                                  );
            gen.builder.CreateCall(gen.pins("llmc_print_chunk"), {data, len});
            gen.builder.CreateCall(gen.pins("printf"), {gen.generateGlobalString("\n")});
            auto vChunkID = ConstantInt::get(gen.t_int, chunkID);
            gen.builder.CreateCall( gen.pins("pins_chunk_put_at")
                                  , {model, ConstantInt::get(gen.t_int, idx)
                                    , chunk, vChunkID
                                    }
                                  );
            return vChunkID;
        }

        /**
         * @brief Gets the chunk specified by @c chunkid and copies it.
         * The copy is done by @c memcpy into a char array alloca'd on the stack.
         * The array Value is returned and the size of the array Value is assigned to @c ch_len.
         * @param chunkid The chunkid of the chunk to get
         * @param ch_len To which the Size of the chunk will be assigned
         * @return Copied chunk
         */
        Value* generateGetAndCopy(Value* chunkid, Value*& ch_len) {
            Value* ch = generateGet(chunkid);
            Value* ch_data = gen.generateChunkGetData(ch);
            ch_len = gen.generateChunkGetLen(ch);
            auto copy = gen.builder.CreateAlloca(gen.t_int8, ch_len);
            gen.builder.CreateMemCpy( copy
                                    , ch_data
                                    , ch_len
                                    , copy->getPointerAlignment(gen.pinsModule->getDataLayout())
                                    );
            gen.builder.CreateCall( gen.pins("printf")
                                  , {gen.generateGlobalString("CHUNK[" + type->_name + "] get of %i bytes:")
                                    , ch_len
                                    }
                                  );
            gen.builder.CreateCall(gen.pins("llmc_print_chunk"), {ch_data, ch_len});
            gen.builder.CreateCall(gen.pins("printf"), {gen.generateGlobalString("\n")});
            return copy;
        }

        /**
         * @brief Clones the chunk specified by @c chunkid, with the specified delta.
         * The delta is specified by @c offset, @c data, and @c len, where
         * @c offset and @c len denote where the clone will differ and @c data
         * denotes how the clone will differ.
         *
         *                v------- offset
         *  Original: --------------------
         *                <-len-->
         *  Data:         xxxxxxxx
         *
         *  Clone:    ----xxxxxxxx--------
         *
         * @param chunkid The chunkID of the chunk to clone
         * @param offset Where the data is to be written in the clone
         * @param data Data to overwrite the clone with
         * @param len Length of @c data
         * @return ChunkID of the cloned chunk
         */
        Value* generateCloneAndModify(Value* chunkid, Value* offset, Value* data, Value* len) {
            assert(0 && "unknown if the CAM version works");
            if(chunkid->getType()->isPointerTy()) {
                chunkid = gen.builder.CreateLoad(chunkid);
            }
            return gen.builder.CreateCall( gen.pins("pins_chunk_cam")
                                         , { model
                                           , ConstantInt::get(gen.t_int, idx)
                                           , chunkid
                                           , offset
                                           , data
                                           , len
                                           }
                                         , "chunkid." + type->_name
                                         );
        }

        // Optimization idea:
        // since the stack is only appended to, when something is popped, we can optimize determining the
        // chunkID the naive way is to just use put_chunk by passing a pointer to the current data, but with a
        // shorter length. Then the sha is determined again. Other approach is to remember the chunkID of the
        // previous stack chunkID. So:
        // stack: y -> [stackChunkID x][frameChunkID 0][frameChunkID 1] ... [frameChunkID n]
        // then a push happens:
        // stack: z -> [stackChunkID y][frameChunkID 0][frameChunkID 1] ... [frameChunkID n][frameChunkID n+1]
        // so when a pop happens, it is fast to determine the previous chunkID, because it is just y

        /**
         * @brief Clone the chunk @c chunkid but with @c data of length @c len
         * appended to it.
         * @param chunkid The chunkID of the chunk to clone
         * @param data The data to append to the clone of the chunk
         * @param len The length of the data to append
         * @return The chunkID of the cloned chunk with the change
         */
        Value* generateCloneAndAppend(Value* chunkid, Value* data, Value* len) {
            Value* ch = generateGet(chunkid);
            Value* chData = gen.generateChunkGetData(ch);
            Value* chLen = gen.generateChunkGetLen(ch);
            Value* newLength = gen.builder.CreateAdd(chLen,len);
            auto copy = gen.builder.CreateAlloca(gen.t_int8, newLength);
            gen.builder.CreateMemCpy( copy
                                    , chData
                                    , chLen
                                    , copy->getPointerAlignment(gen.pinsModule->getDataLayout())
                                    );
            gen.builder.CreateMemCpy( gen.builder.CreateGEP(copy, {chLen})
                                    , data
                                    , len
                                    , copy->getPointerAlignment(gen.pinsModule->getDataLayout())
                                    );
            return generatePut(newLength, copy);
        }

        /**
         * @brief Clone the chunk @c chunkid but with @c data appended to it.
         * The length of the data is determined by the type of @c data.
         * @param chunkid The chunkID of the chunk to clone
         * @param data The data to append to the clone of the chunk
         * @return The chunkID of the cloned chunk with the change
         */
        Value* generateCloneAndAppend(Value* chunkid, Value* data) {
            Value* ch = generateGet(chunkid);
            Value* chData = gen.generateChunkGetData(ch);
            Value* chLen = gen.generateChunkGetLen(ch);

            Value* len = gen.generateSizeOf(data);
            Value* newLength = gen.builder.CreateAdd(chLen,len);
            auto copy = gen.builder.CreateAlloca(gen.t_int8, newLength);
            gen.builder.CreateMemCpy( copy
                                    , chData
                                    , chLen
                                    , copy->getPointerAlignment(gen.pinsModule->getDataLayout())
                                    );
            auto target = gen.builder.CreateGEP(copy, {chLen});
            target = gen.builder.CreatePointerCast(target, data->getType()->getPointerTo());
            gen.builder.CreateStore(data, target);
            return generatePut(newLength, copy);
        }

    };

    /**
     * @class ProcessStack
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Class for helper functions for the stack of a process
     */
    class ProcessStack {
    private:
        LLPinsGenerator& gen;
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

    public:
        ProcessStack(LLPinsGenerator& gen)
        : gen(gen)
        {
        }

        void init() {
            // previous pc, register frame
            t_frame = StructType::create(gen.ctx, {gen.t_int, gen.t_chunkid}, "t_stackframe");
            t_framep = t_frame->getPointerTo();
        }


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
        void pushStackFrame(GenerationContext* gctx, Function& F, std::vector<Value*> const& args, bool setupCall = false) {
            auto& gen = *gctx->gen;
            auto& builder = gen.builder;

            llvmgen::BBComment(builder, "pushStackFrame");

            auto dst_pc = gen.lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);
            auto dst_reg = gen.lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

            // Save the state of the registers and setup parameters
            if(!setupCall) {
                // Create new register frame chunk
                ChunkMapper cm_rframe(gen, gctx->model, gen.type_register_frame);
                Value* chunkid = cm_rframe.generatePut( gen.t_registers_max_size
                                                      , dst_reg
                                                      );

                // Create new frame
                Value* frame = gen.builder.CreateInsertValue( UndefValue::get(t_frame)
                                                            , gen.builder.CreateLoad(dst_pc, "pc")
                                                            , {0}
                                                            );
                frame = gen.builder.CreateInsertValue(frame, chunkid, {1});

                // Put the new frame on the stack
                auto pStackChunkID = gen.lts["processes"][gctx->thread_id]["stk"].getValue(gctx->svout);
                auto stackChunkID = builder.CreateLoad(pStackChunkID, "stackChunkID");
                ChunkMapper cm_stack(gen, gctx->model, gen.type_stack);
                auto newStackChunkID = cm_stack.generateCloneAndAppend( stackChunkID
                                                                      , frame
                                                                      );
                gen.builder.CreateStore(newStackChunkID, pStackChunkID);

            }

            // Check if the number of arguments is correct
            // Too few arguments results in 0-values for the later parameters
            int a = 0;
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
                auto v = gen.vMap(gctx, arg);

                // Map the parameter register to the location in the state vector
                auto vParam = gen.vReg(dst_reg, &*param);

                // Perform the store
                gen.builder.CreateStore(v, vParam);

                // Next
                param++;
            }

            // Set the program counter to the start of the pushed function
            auto loc = gen.programLocations[&*F.getEntryBlock().begin()];
            assert(loc);
            gen.builder.CreateStore(ConstantInt::get(gen.t_int, loc), dst_pc);
        }

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
         */
        void popStackFrame(GenerationContext* gctx) {
            auto& gen = *gctx->gen;
            auto& builder = gen.builder;

            llvmgen::BBComment(builder, "popStackFrame");

            llvmgen::If If(builder, "if_there_are_frames");

            auto dst_pc = gen.lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);

            auto pStackChunkID = gen.lts["processes"][gctx->thread_id]["stk"].getValue(gctx->svout);
            auto stackChunkID = builder.CreateLoad(pStackChunkID, "stackChunkID");
            If.setCond(builder.CreateICmpNE(stackChunkID, ConstantInt::get(gen.t_int, 0)));
            BasicBlock* BBTrue = If.getTrue();
            BasicBlock* BBFalse = If.getFalse();
            If.generate();


            // When there is a frame to be popped
            {
                builder.SetInsertPoint(&*BBTrue->getFirstInsertionPt());

                // Get the stack
                ChunkMapper cm_stack(gen, gctx->model, gen.type_stack);
                Value* chunk = cm_stack.generateGet(stackChunkID);
                Value* chLen = gen.generateChunkGetLen(chunk);
                Value* chData = gen.generateChunkGetData(chunk);

                // Pop the last frame from the stack
                Value* newLength = builder.CreateSub( chLen
                                                    , gen.generateSizeOf(t_frame)
                                                    , "length_without_popped_frame"
                                                    );
                chunk = gen.builder.CreateInsertValue(chunk, newLength, {0});
                auto newStackChunkID = cm_stack.generatePut(chunk);
                builder.CreateStore(newStackChunkID, pStackChunkID);

                // Load the PC of the caller function from thee frame and store that as the new PC
                auto popped_frame = builder.CreateGEP(chData, newLength);
                popped_frame = builder.CreatePointerCast(popped_frame, t_framep);
                auto prevPC = builder.CreateGEP( popped_frame
                                               , { ConstantInt::get(gen.t_int, 0)
                                                 , ConstantInt::get(gen.t_int, 0)
                                                 }
                                               );
                prevPC = builder.CreateLoad(prevPC, "pc");
                gen.builder.CreateStore(prevPC, dst_pc);

                // Restore stored registers
                auto prevRegsChunkID = builder.CreateGEP( popped_frame
                                                        , { ConstantInt::get(gen.t_int, 0)
                                                          , ConstantInt::get(gen.t_int, 1)
                                                          }
                                                        );
                prevRegsChunkID = builder.CreateLoad(prevRegsChunkID, "prevRegsChunkID");

                ChunkMapper cm_rframe(gen, gctx->model, gen.type_register_frame);
                Value* chunkReg = cm_rframe.generateGet(prevRegsChunkID);
                Value* chRegLen = gen.generateChunkGetLen(chunkReg);
                Value* chRegData = gen.generateChunkGetData(chunkReg);

                gen.builder.CreateMemCpy( gen.lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout)
                                        , chRegData
                                        , chRegLen
                                        , chRegData->getPointerAlignment(gen.pinsModule->getDataLayout())
                                        );
            }

            // When there is no frame to be popped
            {
                // Set the PC of this thread to terminated
                builder.SetInsertPoint(&*BBFalse->getFirstInsertionPt());
                gen.builder.CreateStore(ConstantInt::get(gen.t_int, 0), dst_pc);
            }

            // Continue the rest of the code after the If
            builder.SetInsertPoint(If.getFinal());

        }

    };

    /**
     * @class Heap
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Class for helper functions for heap management
     */
    class Heap {
    private:
        GenerationContext* gctx;
        Value* sv;
        Value* memory;
        Value* memory_len;
        Value* memory_data;
        Value* memory_info;
        Value* memory_info_len;
        Value* memory_info_data;
        ChunkMapper cm_memory;
        ChunkMapper cm_memory_info;
    public:

        Heap(GenerationContext* gctx, Value* sv)
        :   gctx(gctx)
        ,   sv(sv)
        ,   memory(nullptr)
        ,   memory_len(nullptr)
        ,   memory_data(nullptr)
        ,   memory_info(nullptr)
        ,   memory_info_len(nullptr)
        ,   memory_info_data(nullptr)
        ,   cm_memory(*gctx->gen, gctx->model, gctx->gen->type_memory)
        ,   cm_memory_info(*gctx->gen, gctx->model, gctx->gen->type_memory_info)
        {
            assert(gctx->thread_id==0 && "this code assumes all heap vars to be in the same pool");
        }

        /**
         * @brief Downloads the memory and memory info chunks using the chunkIDs in the SV
         * After this call, the memory and memory info can be altered.
         * When done altering, call @c upload().
         *
         * This on its own does not change the state-vector, only downloads the
         * chunks specified in the state-vector into alloca'd memory.
         */
        void download() {

            llvmgen::BBComment(gctx->gen->builder, "Heap: download");

            auto svMemory = gctx->gen->lts["processes"][gctx->thread_id]["memory"].getValue(sv);
            auto svMemoryInfo = gctx->gen->lts["processes"][gctx->thread_id]["memory_info"].getValue(sv);

            auto memory = cm_memory.generateGet(svMemory);
            auto memory_info = cm_memory_info.generateGet(svMemoryInfo);
            auto v_memory_data      = gctx->gen->generateChunkGetData(memory);
            auto v_memory_len       = gctx->gen->generateChunkGetLen(memory);
            auto v_memory_info_data = gctx->gen->generateChunkGetData(memory_info);
            auto v_memory_info_len  = gctx->gen->generateChunkGetLen(memory_info);

            auto F = gctx->gen->builder.GetInsertBlock()->getParent();
            if(!memory_data)      memory_data      = addAlloca(v_memory_data->getType()     , F);
            if(!memory_len)       memory_len       = addAlloca(v_memory_len->getType()      , F);
            if(!memory_info_data) memory_info_data = addAlloca(v_memory_info_data->getType(), F);
            if(!memory_info_len)  memory_info_len  = addAlloca(v_memory_info_len->getType() , F);

            gctx->gen->builder.CreateStore(v_memory_data, memory_data);
            gctx->gen->builder.CreateStore(v_memory_len , memory_len);
            gctx->gen->builder.CreateStore(v_memory_info_data, memory_info_data);
            gctx->gen->builder.CreateStore(v_memory_info_len, memory_info_len);
        }

        /**
         * @brief Allocate @c n bytes in the memory-chunk.
         * @param n The number of bytes to allocate
         * @return Value Pointer to the newly allocated memory
         */
        Value* malloc(Value* n) {
            llvmgen::BBComment(gctx->gen->builder, "Heap: malloc");
            llvm::Function* F = gctx->gen->llmc_func("llmc_os_malloc");
            auto vThreadID = ConstantInt::get(F->getFunctionType()->getParamType(4), gctx->thread_id);
            auto vN = gctx->gen->builder.CreateIntCast(n, F->getFunctionType()->getParamType(5), false);
            return gctx->gen->builder.CreateCall(F
                                                , { memory_len
                                                  , memory_data
                                                  , memory_info_len
                                                  , memory_info_data
                                                  , vThreadID
                                                  , vN
                                                  }
                                                );
        }

        /**
         * @brief Allocates memory for the type allocated in the specified
         * AllocaInst @c A.
         * @param A The AllocaInst of which type to allocate memory for
         * @return Value Pointer to the newly allocated memory
         */
        Value* malloc(AllocaInst* A) {
            auto s = gctx->gen->generateSizeOf(A->getAllocatedType());
            return this->malloc(s);
        }

        Value* realloc(Value* p, Value* n) {
            (void)p;
            (void)n;
            assert(0 && "to be implemented");
        }

        void free(Value* p) {
            (void)p;
            assert(0 && "to be implemented");
        }

        /**
         * @brief Uploads the memory and memory info chunks.
         * The resulting chunkIDs are stored in the SV.
         *
         * Changes:
         *   - ["processes"][gctx->thread_id]["memory"]
         *   - ["processes"][gctx->thread_id]["memory_info"]
         */
        void upload() {

            llvmgen::BBComment(gctx->gen->builder, "Heap: upload");

            auto svMemory = gctx->gen->lts["processes"][gctx->thread_id]["memory"].getValue(sv);
            auto svMemoryInfo = gctx->gen->lts["processes"][gctx->thread_id]["memory_info"].getValue(sv);

            auto v_memory_data = gctx->gen->builder.CreateLoad(memory_data, "memory_data");
            auto v_memory_len = gctx->gen->builder.CreateLoad(memory_len, "memory_len");
            auto v_memory_info_data = gctx->gen->builder.CreateLoad(memory_info_data, "memory_info_data");
            auto v_memory_info_len = gctx->gen->builder.CreateLoad(memory_info_len, "memory_info_len");
            auto memory = cm_memory.generatePut(v_memory_len, v_memory_data);
            auto memory_info = cm_memory_info.generatePut(v_memory_info_len, v_memory_info_data);
            gctx->gen->builder.CreateStore(memory, svMemory);
            gctx->gen->builder.CreateStore(memory_info, svMemoryInfo);
        }

    };

    /**
     * @class LLVMLTS
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Class describing the LTS type of the LLVM model,
     */
    class LLVMLTS {
    public:

        LLVMLTS()
        : sv(nullptr)
        , labels(nullptr)
        , edgeLabels(nullptr)
        {
        }

        /**
         * @brief Adds a state-vector node to the root node of the SV tree
         * @param t The SVTree node to add
         * @return *this
         */
        LLVMLTS& operator<<(SVTree* t) {
            (*sv) << t;
            return *this;
        }

        /**
         * @brief Adds a state label node to the LTS type
         * @param t The SVLabel node to add
         * @return *this
         */
        LLVMLTS& operator<<(SVLabel* t) {
            (*labels) << t;
            return *this;
        }

        /**
         * @brief Adds an edge label node to the LTS type
         * @param t The SVEdgeLabel node to add
         * @return *this
         */
        LLVMLTS& operator<<(SVEdgeLabel* t) {
            (*edgeLabels) << t;
            return *this;
        }

        /**
         * @brief Sets the root node of the SV tree
         * @param r The root node to set
         * @return *this
         */
        LLVMLTS& operator<<(SVRoot* r) {
            assert(!sv);
            sv = r;
            return *this;
        }

        /**
         * @brief Sets the root node of the state label tree
         * @param r The root node to set
         * @return *this
         */
        LLVMLTS& operator<<(SVLabelRoot* r) {
            assert(!labels);
            labels = r;
            return *this;
        }

        /**
         * @brief Sets the root node of the edge label tree
         * @param r The root node to set
         * @return *this
         */
        LLVMLTS& operator<<(SVEdgeLabelRoot* r) {
            assert(!edgeLabels);
            edgeLabels = r;
            return *this;
        }

        /**
         * @brief Adds the SVType @c t to the list of types in the LTS
         * @param t the SVType to add
         * @return *this
         */
        LLVMLTS& operator<<(SVType* t) {
            types.push_back(t);
            return *this;
        }

        /**
         * @brief Dumps information on this LTS Type to std::cout
         */
        void dump() {
            auto& o = std::cout;

            std::function<void(SVTree*,int)> pr = [&](SVTree* t, int n) -> void {
                for(int i=n; i--;) {
                    o << "  ";
                }
                o << t->getName();
                if(t->getChildren().size() > 0) {
                    o << ":";
                }
                o << std::endl;
                for(auto& c: t->getChildren()) {
                    pr(c, n+1);
                }
            };

            o << "Types: " << std::endl;
            int n = 0;
            for(auto& t: types) {
                o << "  " << t->index << ": " << t->_name << std::endl;
                n++;
            }

            pr(sv, 0);
        }

        /**
         * @brief Verifies the correctness of this LTS type
         * @return true iff the LTS type passes
         */
        bool verify() {
            bool ok = true;
            if(sv && !sv->verify()) ok = false;
            if(labels && !labels->verify()) ok = false;
            if(edgeLabels && !edgeLabels->verify()) ok = false;
            return ok;
        }

        /**
         * @brief Finalizes this LTS Type. Use this when no more changes
         * are performed. This will determine the index of the types.
         */
        void finish() {
            int n = 0;
            for(auto& t: types) {
                t->index = n++;
            }
        }

        /**
         * @brief Accesses the child of the root SV node with the specified
         * name
         * @param name The name of the child to access.
         * @return The SVTree child node with the specified name.
         */
        SVTree& operator[](std::string name) {
            return (*sv)[name];
        }

        /**
         * @brief Accesses the chil of the root SV node at the specified index
         * @param i The index of the child to access.
         * @return The SVTree child node at the specified index.
         */
        SVTree& operator[](size_t i) {
            return (*sv)[i];
        }

        bool hasSV() const { return sv != nullptr; }
        bool hasLabels() const { return labels != nullptr; }
        bool hasEdgeLabels() const { return edgeLabels != nullptr; }

        SVRoot& getSV() const {
            assert(sv);
            return *sv;
        }

        SVLabelRoot& getLabels() const {
            assert(labels);
            return *labels;
        }

        SVEdgeLabelRoot& getEdgeLabels() const {
            assert(edgeLabels);
            return *edgeLabels;
        }

        std::vector<SVType*>& getTypes() {
            return types;
        }

    private:
        SVRoot* sv;
        SVLabelRoot* labels;
        SVEdgeLabelRoot* edgeLabels;
        std::vector<SVType*> types;
    };

private:
    std::unique_ptr<llvm::Module> up_module;
    LLVMContext& ctx;
    llvm::Module* module;
    llvm::Module* pinsModule;
    std::unique_ptr<llvm::Module> up_pinsHeaderModule;
    std::unique_ptr<llvm::Module> up_libllmcosModule;
    std::vector<TransitionGroup*> transitionGroups;
    llvm::Type* t_void;
    llvm::Type* t_voidp;
    llvm::Type* t_charp;
    llvm::IntegerType* t_int;
    llvm::PointerType* t_intp;
    llvm::IntegerType* t_intptr;
    llvm::IntegerType* t_int8;
    llvm::IntegerType* t_int64;
    llvm::StructType* t_chunk;
    llvm::Type* t_chunkid;
    llvm::StructType* t_model;
    llvm::PointerType* t_modelp;
    llvm::ArrayType* t_registers_max;
    Value* t_registers_max_size;
    llvm::StructType* t_statevector;
    llvm::StructType* t_edgeLabels;
    llvm::FunctionType* t_pins_getnext;
    llvm::FunctionType* t_pins_getnextall;

    static int const MAX_THREADS = 1;
    static int const BITWIDTH_STATEVAR = 32;
    static int const BITWIDTH_INT = 32;
    llvm::Constant* t_statevector_size;

    GlobalVariable* s_statevector;

    llvm::Function* f_pins_getnext;
    llvm::Function* f_pins_getnextall;

    llvm::BasicBlock* f_pins_getnext_end;
    llvm::BasicBlock* f_pins_getnext_end_report;

    llvm::IRBuilder<> builder;

    Value* transition_info;
    std::unordered_map<Instruction*, int> programLocations;
    std::unordered_map<Value*, int> valueRegisterIndex;
    std::unordered_map<Function*, FunctionData> registerLayout;
    int nextProgramLocation;

    MessageFormatter& out;
    raw_os_ostream roout;

    std::unordered_map<std::string, GenerationContext> generationContexts;
    std::unordered_map<std::string, AllocaInst*> allocas;
    std::unordered_map<std::string, GlobalVariable*> generatedStrings;

    LLVMLTS lts;

    SVType* type_status;
    SVType* type_pc;
    SVType* type_stack;
    SVType* type_register_frame;
    SVType* type_registers;
    SVType* type_memory;
    SVType* type_memory_info;
    SVType* type_heap;
    SVType* type_action;

    ProcessStack stack;

    std::unordered_map<std::string, Function*> hookedFunctions;

    bool debugChecks;

public:
    LLPinsGenerator(std::unique_ptr<llvm::Module> modul, MessageFormatter& out)
        : up_module(std::move(modul))
        , ctx(up_module->getContext())
        , t_statevector(nullptr)
        , s_statevector(nullptr)
        , builder(ctx)
        , nextProgramLocation(1)
        , out(out)
        , roout(out.getConsoleWriter().ss())
        , stack(*this)
        {
        module = up_module.get();
        pinsModule = nullptr;
    }

    /**
     * @brief Cleans up this instance.
     */
    void cleanup() {
        if(pinsModule) {
            delete pinsModule;
        }
        pinsModule = nullptr;
    }

    /**
     * @brief Load modules
     */
    std::unique_ptr<Module> loadModule(std::string name) {

        std::vector<File> tries;
        tries.emplace_back(File(System::getBinaryLocation(), name));
        tries.emplace_back(File(std::string(CompileOptions::CMAKE_INSTALL_PREFIX) + "/share/llmc", name));
        tries.emplace_back(File(System::getBinaryLocation() + "/../libllmcos", name));

        File moduleFile;
        for(auto& f: tries) {
            if(f.exists()) {
                moduleFile = f;
                break;
            }
        }

        if(moduleFile.exists()) {
            moduleFile.fix();
            // Load the LLVM module
            SMDiagnostic diag;
            auto module = llvm::parseAssemblyFile(moduleFile.getFilePath(), diag, ctx);
            if(!module) {
                out.reportError("Error oading module " + name + ": " + moduleFile.getFilePath() + ":");
                fflush(stdout);
                printf("=======\n");
                fflush(stdout);
                diag.print(name.c_str(), roout, true, true);
                fflush(stdout);
                printf("=======\n");
                fflush(stdout);
                assert(0 && "oh noes");
            } else {
                out.reportNote("Loaded module " + name + ": " + moduleFile.getFilePath());
            }
            return std::move(module);
        } else {
            out.reportError( "Error loading module "
                           + name
                           + ", file does not exist: `"
                           + moduleFile.getFilePath()
                           + "'"
                           );
            assert(0 && "oh noes");
            return std::unique_ptr<Module>(nullptr);
        }

    }

    /**
     * @brief Starts the pinsification process.
     */
    void pinsify() {
        cleanup();

        pinsModule = new Module("pinsModule", ctx);

        out.reportAction("Loading internal modules...");
        out.indent();
        up_libllmcosModule = loadModule("libllmcos.ll");
        up_pinsHeaderModule = loadModule("pins.h.ll");
        out.outdent();

        // Copy function declarations from the loaded modules into the
        // module of the model we are now generating such that we can call
        // these functions
        for(auto& f: up_pinsHeaderModule->getFunctionList()) {
            if(!pinsModule->getFunction(f.getName().str())) {
                Function::Create(f.getFunctionType(), f.getLinkage(), f.getName(), pinsModule);
            }
        }
        out.reportAction("Instrumenting hooks");
        for(auto& f: up_libllmcosModule->getFunctionList()) {
            auto const& functionName = f.getName().str();
            if(!pinsModule->getFunction(functionName)) {
                auto F = Function::Create(f.getFunctionType(), f.getLinkage(), f.getName(), pinsModule);
                out.indent();
                if(!functionName.compare(0, 10, "llmc_hook_")) {
                    std::string name = functionName.substr(10);
                    hookedFunctions[name] = F;
                    out.reportNote("Hooking " + name + " to " + functionName);
                }
                out.outdent();
            }
        }

        // Determine all the transition groups from the code
        createTransitionGroups();

        // Generate the LLVM types we need
        generateTypes();

        // Generate the LLVM functions that we will populate later
        generateSkeleton();

        // Create the register mapping used to map registers to locations
        // in the state-vector
        createRegisterMapping();

        // Generate the initial state
        generateInitialState();

        // Initialize the stack helper functions
        stack.init();

        // Generate the contents of the next state function
        // for...
        generateNextState(0);

        // Generate Popt interfacee
        generateInterface();

        generateDebugInfo();


        //pinsModule->dump();

        out.reportAction("Verifying module...");
        out.indent();

        if(verifyModule(*pinsModule, &roout)) {
            out.reportFailure("Verification failed");
        } else {
            out.reportSuccess("Module OK");
        }
        out.outdent();
    }

    /**
     * @class DebugInfo
     * @author Freark van der Berg
     * @date 04/07/17
     * @file LLPinsGenerator.h
     * @brief Debug info that will be attached to every generated LLVM
     * instruction, such that we can determine which instructions crashed
     * using LLDB.
     */
    struct DebugInfo {
      DICompileUnit *TheCU;
      DISubprogram *getnext;
      DIType *DblTy;
      std::vector<DIScope *> LexicalBlocks;

      DIType *getDoubleTy();
    };

    /**
     * @brief Attach debug info to all generated instructions.
     */
    void generateDebugInfo() {
        pinsModule->addModuleFlag(Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);

        DIBuilder dbuilder(*pinsModule);

        DebugInfo dbgInfo;
#if LLVM_VERSION_MAJOR < 4
        dbgInfo.TheCU = dbuilder.createCompileUnit( dwarf::DW_LANG_hi_user
                                                  , pinsModule->getName(), ".", "llmc", false, "", 0);
#else
        auto file = dbuilder.createFile(pinsModule->getName(), ".");
        dbgInfo.TheCU = dbuilder.createCompileUnit(dwarf::DW_LANG_hi_user, file, "llmc", false, "", 0);
#endif
        DITypeRefArray a = dbuilder.getOrCreateTypeArray({});
        auto d_getnext = dbuilder.createSubroutineType(a);
        dbgInfo.getnext = dbuilder.createFunction( dbgInfo.TheCU
                                                 , "getnext"
                                                 , ""
                                                 , dbgInfo.TheCU->getFile()
                                                 , 0
                                                 , d_getnext
                                                 , false
                                                 , true
                                                 , 0
                                                 );

        dbuilder.finalize();

        int l = 1;
        for(auto& F: *pinsModule) {
            for(auto& BB: F) {
                for(Instruction& I: BB) {
                    I.setDebugLoc(DILocation::get(ctx, l, 1, dbgInfo.getnext, nullptr));
                    l++;
                }
            }
        }

        NamedMDNode* culistOld = module->getNamedMetadata("llvm.dbg.cu");
        NamedMDNode* culist = pinsModule->getNamedMetadata("llvm.dbg.cu");

        if(culistOld) {
            assert(culist);
            for(auto cuOld: culistOld->operands()) {
                bool found = false;
                for(auto cu: culist->operands()) {
                    if(cu == cuOld) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    culist->addOperand(cuOld);
                }
            }
        }
    }

    /**
     * @brief Generates code to get the length of a chunk from @c chunk
     * @param chunk Chunk of which to get the length
     * @return The length of the chunk
     */
    Value* generateChunkGetLen(Value* chunk) {
        return builder.CreateExtractValue(chunk, 0, chunk->getName() + ".len");
    }

    /**
     * @brief Generates code to get the data of a chunk from @c chunk
     * @param chunk Chunk of which to get the data
     * @return The data of the chunk
     */
    Value* generateChunkGetData(Value* chunk) {
        return builder.CreateExtractValue(chunk, 1, chunk->getName() + ".data");
    }

    /**
     * @brief Generates code that determines the size of the type of the
     * specified Value.
     * @param data The Value of which the size of the type will be determined.
     * @return The size of the type of the specified Value.
     */
    Constant* generateSizeOf(Value* data) {
        return generateSizeOf(data->getType());
    }

    /**
     * @brief Generates code that determines the size of the specified type.
     * @param type The type of which the size will be determined.
     * @return The size of the type.
     */
    Constant* generateSizeOf(Type* type) {
        auto s = builder.getFolder().CreateGetElementPtr( type
                                                        , ConstantPointerNull::get(type->getPointerTo(0))
                                                        , {ConstantInt::get(t_int, 1)}
                                                        );
        s = builder.getFolder().CreatePtrToInt(s, t_int);
        return s;
    }

    /**
     * @brief Generates all the callbacks to implement the PINS interface to
     * LTSmin.
     *
     * This sets a number of f_* functions:
     *   - f_pins_getnextall
     *   - f_pins_getnext
     */
    void generateCallbacks() {
        assert(pinsModule);

        f_pins_getnextall = Function::Create( t_pins_getnextall
                                            , GlobalValue::LinkageTypes::ExternalLinkage
                                            , "pins_getnextall"
                                            , pinsModule
                                            );
        f_pins_getnext = Function::Create( t_pins_getnext
                                         , GlobalValue::LinkageTypes::ExternalLinkage
                                         , "pins_getnext"
                                         , pinsModule
                                         );

        // This is a very crude implementation of getNextAll(), because
        if(f_pins_getnextall) {

            // Get the arguments from the call
            auto args = f_pins_getnextall->arg_begin();
            Argument* self = &*args++;
            Argument* src = &*args++;
            Argument* cb = &*args++;
            Argument* user_context = &*args++;

            // Basic blocks needed for the for loop
            BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_pins_getnextall);
            BasicBlock* forcond = BasicBlock::Create(ctx, "for_cond" , f_pins_getnextall);
            BasicBlock* forbody = BasicBlock::Create(ctx, "for_body" , f_pins_getnextall);
            BasicBlock* forincr = BasicBlock::Create(ctx, "for_incr" , f_pins_getnextall);
            BasicBlock* end = BasicBlock::Create(ctx, "end" , f_pins_getnextall);

            // From the entry, we jump to the condition
            builder.SetInsertPoint(entry);
            builder.CreateBr(forcond);

            // The condition checks that the current transition group (tg) is
            // smaller than the total number of transition groups
            builder.SetInsertPoint(forcond);
            PHINode* tg = builder.CreatePHI(t_int, 2, "TG");
            Value* cond = builder.CreateICmpULT(tg, ConstantInt::get(t_int, transitionGroups.size()));
            builder.CreateCondBr(cond, forbody, end);

            // The body of the for loop: calls pins_getnext for very group
            builder.SetInsertPoint(forbody);
            builder.CreateCall(f_pins_getnext, {self, tg, src, cb, user_context});
            builder.CreateBr(forincr);

            // Increment tg
            builder.SetInsertPoint(forincr);
            Value* nextTG = builder.CreateAdd(tg, ConstantInt::get(t_int, 1));
            builder.CreateBr(forcond);

            // Tell the Phi node of the incoming edges
            tg->addIncoming(ConstantInt::get(t_int, 0), entry);
            tg->addIncoming(nextTG, forincr);

            // Finally return 0
            builder.SetInsertPoint(end);
            builder.CreateRet(ConstantInt::get(t_int, 0));

        }

        if(f_pins_getnext) {

            // Get the arguments from the call
            auto args = f_pins_getnext->arg_begin();
            Argument* self = &*args++;
            Argument* grp = &*args++;
            Argument* src = &*args++;
            Argument* cb = &*args++;
            Argument* user_context = &*args++;

            // Create entry block with a number of stack allocations
            BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_pins_getnext);
            builder.SetInsertPoint(entry);

            // FIXME: should not be 'global'
            transition_info = builder.CreateAlloca(pins_type("transition_info_t"));
            auto edgeLabelValue = builder.CreateAlloca(t_edgeLabels);
            auto svout = builder.CreateAlloca(t_statevector);

            // Set the context that is used for generation
            GenerationContext gctx = {};
            gctx.gen = this;
            gctx.model = self;
            gctx.src = src;
            gctx.svout = svout;
            gctx.edgeLabelValue = edgeLabelValue;
            gctx.alteredPC = false;
            generationContexts["pins_getnext"] = gctx;

            // Basic Blocks
            BasicBlock* doswitch = BasicBlock::Create(ctx, "doswitch" , f_pins_getnext);
            BasicBlock* end = BasicBlock::Create(ctx, "end" , f_pins_getnext);
            BasicBlock* end_report = BasicBlock::Create(ctx, "end_report" , f_pins_getnext);
            BasicBlock* nosuchgroup = BasicBlock::Create(ctx, "nosuchgroup" , f_pins_getnext);

            // Set the 'global' end basic blocks to be used in generation
            // FIXME: could be part of GenerationContext
            f_pins_getnext_end = end;
            f_pins_getnext_end_report = end_report;

            // FIXME: decide if memset is wanted or just init when needed
            auto transition_info_size = generateSizeOf(pins_type("transition_info_t"));
            builder.CreateMemSet( transition_info
                                , ConstantInt::get(t_int8, 0)
                                , transition_info_size
                                , transition_info->getPointerAlignment(pinsModule->getDataLayout())
                                );
            builder.CreateMemSet( edgeLabelValue
                                , ConstantInt::get(t_int8, 0)
                                , generateSizeOf(t_edgeLabels)
                                , edgeLabelValue->getPointerAlignment(pinsModule->getDataLayout())
                                );

            // Basic block for when the specified group ID is invalid
            builder.SetInsertPoint(nosuchgroup);
            builder.CreateBr(end);

            // Create the switch (we branch to this later)
            builder.SetInsertPoint(doswitch);
            SwitchInst* swtch = builder.CreateSwitch(grp, nosuchgroup, transitionGroups.size());

            // Per transition group, add an entry to the switch, the bodies of
            // the cases will be populated later
            for(size_t i = 0; i < transitionGroups.size(); ++i) {
                auto& t = transitionGroups[i];
                BasicBlock* grpCode = BasicBlock::Create(ctx, "groupcode" , f_pins_getnext);
                t->nextState = grpCode;
                swtch->addCase(ConstantInt::get(t_int, i), grpCode);
                //builder.SetInsertPoint(grpCode);
            }

            // BB for when a transition group finds a transition
            builder.SetInsertPoint(end_report);
            builder.CreateCall(pins("printf"), {generateGlobalString("[%2i] found a new transition\n"), grp});

            // FIXME: mogelijk gaat edgeLabelValue fout wanneer 1 label niet geset is maar een andere wel

            builder.CreateCall( cb
                              , { user_context
                                , transition_info
                                , builder.CreatePointerCast(svout, t_intp)
                                , ConstantPointerNull::get(t_intp)
                                }
                              );
            builder.CreateRet(ConstantInt::get(t_int, 1));

            // BB for when a transition group does not find a transition
            builder.SetInsertPoint(end);
            //builder.CreateCall(pins("printf"), {generateGlobalString("[%2i] no\n"), grp});
            builder.CreateRet(ConstantInt::get(t_int, 0));

            // From entry, branch to the switch
            builder.SetInsertPoint(entry);
            builder.CreateBr(doswitch);

        }
    }

    void createRegisterMapping() {

        unsigned int nextID = 0;
        auto newmap = [&nextID, this](Function* F, Value* V) {
            registerLayout[F].registerTypes.push_back(V->getType());
            valueRegisterIndex[V] = nextID++;
        };

        // Globals
        // Assign every global an index, starting at 0
        for(auto& v: module->getGlobalList()) {
            valueRegisterIndex[&v] = nextID++;
        }

        // Store the register layout of all the functions in the program,
        // such that we can map registers to a location in the SV
        for(auto& F: *module) {
            if(F.isDeclaration()) continue;

            // Reset the ID
            nextID = 0;

            // Map the arguments
            for(auto& A: F.args()) {
                newmap(&F, &A);
            }

            // Map the registers
            for(auto& BB: F) {
                for(auto& I: BB) {
                    if(!I.getType()->isVoidTy()) {
                        newmap(&F, &I);
                    }
                }
            }

            // Create the type
            registerLayout[&F].registerLayout = StructType::create( ctx
                                                                  , registerLayout[&F].registerTypes
                                                                  , "t_" + F.getName().str() + "_registers"
                                                                  );
            assert(nextID == registerLayout[&F].registerTypes.size());
        }
    }

    /**
     * @brief Generates the skeleton of the pinsified module
     */
    void generateSkeleton() {
        generateCallbacks();
    }

    /**
     * @brief Generates the types to be used and generates the LTS type
     */
    void generateTypes() {
        assert(t_statevector == nullptr);

        // Basic types
        t_int = IntegerType::get(ctx, BITWIDTH_STATEVAR);
        t_intp = t_int->getPointerTo(0);
        t_int8 = IntegerType::get(ctx, 8);
        t_int64 = IntegerType::get(ctx, 64);
        t_intptr = IntegerType::get(ctx, 64); // FIXME
        t_void = PointerType::getVoidTy(ctx);
        t_charp = PointerType::get(IntegerType::get(ctx, 8), 0);
        t_voidp = t_charp; //PointerType::get(PointerType::getVoidTy(ctx), 0);

        // Chunk types
        t_chunkid = IntegerType::getInt32Ty(ctx);
        t_chunk = dyn_cast<StructType>(pins("pins_chunk_get")->getReturnType());

        // Determine the register size
        t_registers_max = ArrayType::get(t_int, 20); // needs to be based on max #registers of all functions
        t_registers_max_size = builder.CreateGEP( t_registers_max
                                                , ConstantPointerNull::get(t_registers_max->getPointerTo())
                                                , {ConstantInt::get(t_int, 1)}
                                                );
        t_registers_max_size = builder.CreatePtrToInt(t_registers_max_size, t_int);

        // Creates types to use in the LTS type
        type_status = new SVType( "status"
                                 , LTStypeSInt32
                                 , t_int
                                 , this
                                 );
        type_pc = new SVType( "pc"
                                 , LTStypeSInt32
                                 , t_int
                                 , this
                                 );
        type_stack = new SVType( "stack"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );
        type_register_frame = new SVType( "rframe"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );
        type_registers = new SVType( "register"
                                 , LTStypeSInt32
                                 , t_registers_max
                                 , this
                                 );
        type_memory = new SVType( "memory"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );
        type_memory_info = new SVType( "minfo"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );
        type_heap = new SVType( "heap"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );
        type_action = new SVType( "action"
                                 , LTStypeChunk
                                 , t_chunkid
                                 , this
                                 );

        // Add the types to the LTS type
        lts << type_status;
        lts << type_pc;
        lts << type_stack;
        lts << type_register_frame;
        lts << type_registers;
        lts << type_memory;
        lts << type_memory_info;
        lts << type_heap;
        lts << type_action;

        // Create the state-vector tree of variable nodes
        auto sv = new SVRoot(this, "type_sv");

        // Processes
        auto sv_processes = new SVTree("processes", "processes");
        for(int i=0; i < MAX_THREADS; ++i) {
            auto sv_proc = new SVTree("", "process");
            *sv_proc
                << new SVTree("status", type_status)
                << new SVTree("pc", type_pc)
                << new SVTree("memory", type_memory)
                << new SVTree("memory_info", type_memory_info)
                << new SVTree("stk", type_stack)
                << new SVTree("r", type_registers)
                ;
            *sv_processes << sv_proc;
        }

        // Globals
        auto sv_globals = new SVTree("globals", "globals");
        for(auto& t: module->getGlobalList()) {

            // Determine the type of the global
            PointerType* pt = dyn_cast<PointerType>(t.getType());
            assert(pt && "global is not pointer... constant?");
            auto gt = pt->getElementType();

            // Add the global
            auto type_global = new SVType( "global_" + t.getName().str()
                                         , LTStypeSInt32
                                         , gt
                                         , this
                                         );
            lts << type_global;
            *sv_globals << new SVTree(t.getName(), type_global);
        }

        // Heap
        auto sv_heap = new SVTree("heap", type_heap);

        // State-vector specification
        *sv
            << new SVTree("status", type_status)
            << sv_processes
            << sv_globals
            << sv_heap
            ;

        // Edge label specification
        auto edges = new SVEdgeLabelRoot(this, "type_edgelabels");
        *edges
            << new SVTree("action", type_action)
            ;

        // State label specification
//        auto labels = new SVLabelRoot(this, "label");
//        *labels
//            ;

        // Add the specifications to the LTS type
        lts << sv;
//        lts << labels;
        lts << edges;
        lts.finish();

        out.reportAction("Verifying LTS...");
        out.indent();

        if(lts.verify()) {
            out.reportSuccess("Module OK");
        } else {
            lts.dump();
            out.reportFailure("Verification failed");
        }
        out.outdent();

        // Get the struct types
        t_statevector = dyn_cast<StructType>(sv->getLLVMType());
        t_edgeLabels = dyn_cast<StructType>(edges->getLLVMType());
        assert(t_statevector);
        assert(t_edgeLabels);

        // int (model_t self, int const* src, TransitionCB cb, void* user_context)
        t_pins_getnextall = FunctionType::get( IntegerType::get(ctx, BITWIDTH_STATEVAR)
                                             , { pins_type("grey_box_model")->getPointerTo()
                                               , PointerType::get(t_statevector, 0)
                                               , pins_type("TransitionCB")
                                               , t_voidp
                                               }
                                             , false
                                             );

        // int (model_t self, int group, int const* src, TransitionCB cb, void* user_context)
        t_pins_getnext = FunctionType::get( IntegerType::get(ctx, BITWIDTH_STATEVAR)
                                          , { pins_type("grey_box_model")->getPointerTo()
                                            , IntegerType::get(ctx, BITWIDTH_STATEVAR)
                                            , PointerType::get(t_statevector, 0)
                                            , pins_type("TransitionCB")
                                            , t_voidp
                                            }
                                          , false
                                          );

        // Get the size of the SV
        t_statevector_size = generateSizeOf(t_statevector);
        assert(t_statevector_size);
    }

    void generateInitialState() {
        s_statevector = new GlobalVariable( *pinsModule
                                          , t_statevector
                                          , false
                                          , GlobalValue::ExternalLinkage
                                          , ConstantAggregateZero::get(t_statevector)
                                          , "s_initstate"
                                          );
    }

    /**
     * @brief Generate the next-state function for the specified thread location
     * @param thread_id
     */
    void generateNextState(int thread_id) {

        // Get the arguments
        auto args = f_pins_getnext->arg_begin();
        Argument* self = &*args++;
        Argument* grp = &*args++;
        Argument* src = &*args++;
        Argument* cb = &*args++;
        Argument* user_context = &*args++;

        (void)self;
        (void)cb;
        (void)user_context;

        // Make a copy of the generation context
        GenerationContext gctx_copy = generationContexts["pins_getnext"];
        gctx_copy.thread_id = thread_id;
        GenerationContext* gctx = &gctx_copy;

        assert(src == gctx->src);


        // For all transitions groups
        for(auto& t: transitionGroups) {

            // Reset the generation context
            gctx->edgeLabels.clear();
            gctx->alteredPC = false;

            // If the transition group (TG) models an instruction
            if(t->getType() == TransitionGroup::Type::Instruction) {
                auto ti = static_cast<TransitionGroupInstruction*>(t);

                // Set the insertion point to the point in the switch
                builder.SetInsertPoint(ti->nextState);

                // Create BBs
                auto bb_transition = BasicBlock::Create(ctx, "bb_transition", f_pins_getnext);
                auto bb_end = BasicBlock::Create(ctx, "bb_end", f_pins_getnext);

                // Access the PCs
                auto src_pc = lts["processes"][0]["pc"].getValue(gctx->src);
                auto dst_pc = lts["processes"][0]["pc"].getValue(gctx->svout);

                // Check that the PC in the SV matches the PC of this TG
                auto pc = builder.CreateLoad(src_pc, "pc");
                auto pc_check = builder.CreateICmpEQ( pc
                                                    , ConstantInt::get(t_int, programLocations[ti->srcPC])
                                                    );
                builder.CreateCondBr(pc_check, bb_transition, bb_end);

                // If the PC is a match
                {
                    builder.SetInsertPoint(bb_transition);
                    assert(t_statevector_size);

                    // Copy the SV into a local one
                    builder.CreateMemCpy( gctx->svout
                                        , gctx->src
                                        , t_statevector_size
                                        , src->getParamAlignment()
                                        );

                    // Update the destination PC
                    builder.CreateStore(ConstantInt::get(t_int, programLocations[ti->dstPC]), dst_pc);

                    // Generate the next state for the instruction
                    generateNextStateForInstruction(gctx, ti->srcPC);

                    // Generate the next state for all other actions
                    for(auto& action: ti->actions) {
                        generateNextStateForInstruction(gctx, action);
                    }

                    // DEBUG: print labels
                    for(auto& nameValue: gctx->edgeLabels) {
                        auto value = lts.getEdgeLabels()[nameValue.first].getValue(gctx->edgeLabelValue);
                        builder.CreateStore(nameValue.second, value);
                        builder.CreateCall( pins("printf")
                                          , { generateGlobalString("label: %i at %p\n")
                                            , nameValue.second
                                            , value
                                            }
                                          );
                    }

                    // Store the edge labels in the transition info
                    auto tinfolabels = builder.CreateGEP( pins_type("transition_info_t")
                                                        , transition_info
                                                        , { ConstantInt::get(t_int, 0)
                                                          , ConstantInt::get(t_int, 0)
                                                          }
                                                        );
                    auto edgelabels = builder.CreatePointerCast(gctx->edgeLabelValue, t_intp);
                    builder.CreateStore(edgelabels, tinfolabels);

                    // DEBUG: print
                    std::string s = "[%2i] transition (pc:%i) %3i -> %3i [" + t->desc + "] (%p)\n";
                    builder.CreateCall( pins("printf")
                                      , { generateGlobalString(s)
                                        , grp
                                        , pc
                                        , ConstantInt::get(t_int, programLocations[ti->srcPC])
                                        , ConstantInt::get(t_int, programLocations[ti->dstPC])
                                        , edgelabels
                                        }
                                      );
                    builder.CreateBr(f_pins_getnext_end_report);
                }

                // If the PC is not a match
                {
                    builder.SetInsertPoint(bb_end);
                    builder.CreateCall( pins("printf")
                                      , { generateGlobalString("no: %i %i\n")
                                        , pc
                                        , ConstantInt::get(t_int, programLocations[ti->srcPC])
                                        }
                                      );
                    builder.CreateBr(f_pins_getnext_end);
                }

            // If the TG is a lambda
            } else if(t->getType() == TransitionGroup::Type::Lambda) {
                auto tl = static_cast<TransitionGroupLambda*>(t);

                // Execute it
                tl->execute(gctx);
            }

        }
    }

    /**
     * @brief Creates transition groups that all pinsified modules need, like
     * the call to main() to start the program
     */
    void createDefaultTransitionGroups() {

        // Generate a TG to start main()
        addTransitionGroup(new TransitionGroupLambda([=](GenerationContext* gctx, TransitionGroupLambda* t) {
            assert(t->nextState);

            // Get the arguments
            auto F = builder.GetInsertBlock()->getParent();
            auto args = F->arg_begin();
            Argument* self = &*args++;
            Argument* grp = &*args++;
            Argument* src = &*args++;
            Argument* cb = &*args++;
            Argument* user_context = &*args++;

            (void)self;
            (void)grp;
            (void)cb;
            (void)user_context;

            // Set insertion point to the case of the switch
            builder.SetInsertPoint(t->nextState);

            // Generate the check that all threads are inactive and main()
            // has not been called before
            Value* cond = lts["status"].getValue(gctx->src);
            cond = builder.CreateLoad(t_int, cond, "status");
            cond = builder.CreateICmpEQ(cond, ConstantInt::get(t_int, 0));
            for(int i = 0; i < MAX_THREADS; ++i) {
                auto src_pc = lts["processes"][i]["pc"].getValue(gctx->src);
                auto pc = builder.CreateLoad(t_int, src_pc, "pc");
                auto cond2 = builder.CreateICmpEQ(pc, ConstantInt::get(t_int, 0));
                cond = builder.CreateAnd(cond, cond2);
            }

            BasicBlock* main_start  = BasicBlock::Create(ctx, "main_start" , f_pins_getnext);
            builder.CreateCondBr(cond, main_start, f_pins_getnext_end);

            builder.SetInsertPoint(main_start);
            builder.CreateCall(pins("printf"), {generateGlobalString("main_start\n")});
            builder.CreateMemCpy(gctx->svout, src, t_statevector_size, src->getParamAlignment());
            stack.pushStackFrame(gctx, *module->getFunction("main"), {}, true);
            //builder.CreateStore(ConstantInt::get(t_int, 1), svout_pc[0]);
            builder.CreateStore(ConstantInt::get(t_int, 1), lts["status"].getValue(gctx->svout));
            builder.CreateBr(f_pins_getnext_end_report);

        }
        ));

    }

    /**
     * @brief Create the transition groups for the model
     */
    void createTransitionGroups() {

        // Generate transition groups for non-code related transitions,
        // such as the initial call to main
        createDefaultTransitionGroups();

        //module->dump();
        out << "Transition groups..." << std::endl;
        out.indent();
        for(auto& F: *module) {
            if(F.isDeclaration()) continue;
            out << "Function [" << F.getName().str() << "]" <<  std::endl;
            out.indent();
            createTransitionGroupsForFunction(F);
            out.outdent();
        }
        out.outdent();
        out << "groups: " << transitionGroups.size() <<  std::endl;
    }

    /**
     * @brief Create transition groups for the function @c F
     * @param F The LLVM Function to create transition groups for
     */
    void createTransitionGroupsForFunction(Function& F) {
        for(auto& BB: F) {
            createTransitionGroupsForBasicBlock(BB);
        }
    }

    /**
     * @brief Create transition groups for the BasicBlock @c BB
     * @param BB The LLVM BasicBlock to create transition groups for
     */
    void createTransitionGroupsForBasicBlock(BasicBlock& BB) {
        for(auto& I: BB) {
            createTransitionGroupsForInstruction(I);
        }
    }

    /**
     * @brief Create transition groups for the Instruction @c I
     * @param I The LLVM Instruction to create transition groups for
     */
    void createTransitionGroupsForInstruction(Instruction& I) {

        // Assign a PC to the instruction if it does not have one yet
        if(programLocations[&I] == 0) {
            programLocations[&I] = nextProgramLocation++;
        }

        // Assign a PC to the next instruction if it does not have one yet
        auto II = I.getIterator();
        II++;
        if(programLocations[&*II] == 0) {
            programLocations[&*II] = nextProgramLocation++;
        }

        // Generate a description
        std::string desc;
        raw_string_ostream rdesc(desc);
        rdesc << I;

        // Create the TG
        auto tg = new TransitionGroupInstruction(&I, &*II, {}, {});
        tg->setDesc(rdesc.str());
        addTransitionGroup(tg);

        // Print
        out << "  " << programLocations[&I] << " -> " << programLocations[&*II] << " ";
        I.print(roout, true);
        out << std::endl;
    }

    /**
     * @brief Adds the transition group @c tg to the model
     * @param tg The transition group to add
     */
    void addTransitionGroup(TransitionGroup* tg) {
        transitionGroups.push_back(tg);
    }

    /**
     * @brief Provides a mapping from program-register to model-register.
     * It injects a GEP instruction, so this should be used during generation.
     * @param registers The registers Value in the state-vector
     * @param reg The program-register of which the model-register is wanted
     * @return The model-register assocated with the specified program-register
     */
    Value* vReg(Value* registers, Use* reg) {
        return vReg(registers, reg->get());
    }

    /**
     * @brief Provides a mapping from program-register to model-register.
     * It injects a GEP instruction, so this should be used during generation.
     * @param registers The registers Value in the state-vector
     * @param reg The program-register of which the model-register is wanted
     * @return The model-register assocated with the specified program-register
     */
    Value* vReg(Value* registers, Value* reg) {
        if(auto v = dyn_cast<Instruction>(reg)) {
            Function& F = *v->getParent()->getParent();
            return vReg(registers, F, reg->getName(), v);
        }
        if(auto v = dyn_cast<Argument>(reg)) {
            Function& F = *v->getParent();
            return vReg(registers, F, "Arg" + reg->getName().str(), v);
        }
        assert(0 && "Value not an instruction or argument!");
        return nullptr;
    }

    /**
     * @brief Provides a mapping from program-register to model-register.
     * It injects a GEP instruction, so this should be used during generation.
     * @param registers The registers Value in the state-vector
     * @param F The function where the program-register is from
     * @param name A name to give the resulting register in the pins module
     * @param reg The program-register of which the model-register is wanted
     * @return The model-register assocated with the specified program-register
     */
    Value* vReg(Value* registers, Function& F, std::string name, Value* reg) {
        auto regs = builder.CreateBitOrPointerCast( registers
                                                  , PointerType::get(registerLayout[&F].registerLayout, 0)
                                                  , F.getName().str() + "_registers"
                                                  );
        auto iIdx = valueRegisterIndex.find(reg);
        assert(iIdx != valueRegisterIndex.end());
        auto idx = iIdx->second;

        auto v = builder.CreateGEP( regs
                                  , { ConstantInt::get(t_int, 0)
                                    , ConstantInt::get(t_int, idx)
                                    }
                                  , F.getName().str() + "_" + name
                                  );
        return v;
    }

    Value* vMap(GenerationContext* gctx, Value* OI) {

        // Leave constants as they are
        if(auto v = dyn_cast<GlobalVariable>(OI)) {
            auto idx = valueRegisterIndex[OI];
            return lts["globals"][idx].getValue(gctx->svout);
        } else if(auto ce = dyn_cast<ConstantExpr>(OI)) {
            std::vector<Value*> args;
            for(int i = 0, e = ce->getNumOperands(); i < e; ++i) {
                auto child = ce->getOperand(i);
                Value* vChild = vMap(gctx, child);
                args.push_back(vChild);
            }
            Value* result = nullptr;
            switch(ce->getOpcode()) {
            case Instruction::GetElementPtr:
                result = builder.CreateGEP(args[0], ArrayRef<Value*>(&*args.begin()+1, &*args.end()));
                break;
            default:
                assert(0);
            }
            return result;
        } else if(dyn_cast<Constant>(OI)) {
            return OI;

        // If we have a known mapping, perform the mapping
        } else if(valueRegisterIndex.count(OI) > 0) {
            Value* registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            return builder.CreateLoad(vReg(registers, OI));

        // Otherwise, report an error
        } else {
            roout << "Could not find mapping for " << *OI << "\n";
            roout.flush();
            assert(0);
            return OI;
        }
    }

    /**
     * @brief Change all the registers used in the instruction to the mapped
     * registers using the vreg() mapping and loading the value from the
     * state-vector.
     * @param IC The instruction of which the operands are to be mapped
     */
    void vMapOperands(GenerationContext* gctx, Instruction* IC) {

        // This is because the registers are in the state-vector
        // and thus in-memory, thus they need to be loaded.
        for (auto& OI: IC->operands()) {
            OI = vMap(gctx, OI);
        }
    }

    /**
     * @brief Generates the next-state relation for the Instruction @c I
     * This should be done duing generation, as the instruction is inserted
     * at the current insertion point of the builder.
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    void generateNextStateForInstructionValueMapped(GenerationContext* gctx, Instruction* I) {

        // Clone the instruction
        Instruction* IC = I->clone();

        // Change all the registers used in the instruction to the mapped
        // registers using the vMap() mapping and loading the value from the
        // state-vector. This is because the registers are in the state-vector
        // and thus in-memory, thus they need to be loaded.
        vMapOperands(gctx, IC);

        // Insert the instruction
        builder.Insert(IC);

        // If the instruction has a return value, store the result in the SV
        if(IC->getType() != t_void) {
            assert(valueRegisterIndex[I]);
            auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            builder.CreateStore(IC, vReg(registers, I));
        }
    }

    /**
     * @brief Generates the next-state relation for the Instruction @c I
     * This is the general function to call for generating the next-state
     * relation for an instruction.
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    void generateNextStateForInstruction(GenerationContext* gctx, Instruction* I) {

        // Debug
        std::string str;
        raw_string_ostream strout(str);
        if(valueRegisterIndex.count(I)) {
            strout << "r[" << valueRegisterIndex[I] << "] ";
        }
        strout << *I;
        llvmgen::BBComment(builder, "Instruction: " + strout.str());

        // Generate the action label
        ChunkMapper cm_action(*this, gctx->model, type_action);
        Value* actionLabelChunkID = cm_action.generatePut( ConstantInt::get(t_int, strout.str().length())
                                                         , generateGlobalString(strout.str())
                                                         );
        gctx->edgeLabels["action"] = actionLabelChunkID;

        // Handle the instruction more specifically
        switch(I->getOpcode()) {
            case Instruction::Alloca:
                return generateNextStateForInstruction(gctx, dyn_cast<AllocaInst>(I));
            case Instruction::Load:
                return generateNextStateForMemoryInstruction(gctx, dyn_cast<LoadInst>(I));
            case Instruction::Store:
                return generateNextStateForMemoryInstruction(gctx, dyn_cast<StoreInst>(I));
            case Instruction::Ret:
                return generateNextStateForInstruction(gctx, dyn_cast<ReturnInst>(I));
            case Instruction::Br:
                return generateNextStateForInstruction(gctx, dyn_cast<BranchInst>(I));
            case Instruction::Switch:
            case Instruction::IndirectBr:
            case Instruction::Invoke:
            case Instruction::Resume:
            case Instruction::Unreachable:
            case Instruction::CleanupRet:
            case Instruction::CatchRet:
            case Instruction::CatchSwitch:
            case Instruction::GetElementPtr:
            case Instruction::Fence:
            case Instruction::AtomicCmpXchg:
            case Instruction::AtomicRMW:
            case Instruction::CleanupPad:
            case Instruction::PHI:
                return;
            case Instruction::Call:
                return generateNextStateForCall(gctx, dyn_cast<CallInst>(I));
            case Instruction::VAArg:
            case Instruction::ExtractElement:
            case Instruction::InsertElement:
            case Instruction::ShuffleVector:
            case Instruction::ExtractValue:
            case Instruction::InsertValue:
            case Instruction::LandingPad:
                roout << "Unsupported instruction: " << *I << "\n";
                return;
            default:
                break;
        }

        // The default behaviour
        return generateNextStateForInstructionValueMapped(gctx, I);
    }

    /**
     * @brief Generates the next-state relation for the ReturnInst @c I
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    void generateNextStateForInstruction(GenerationContext* gctx, ReturnInst* I) {
        stack.popStackFrame(gctx);
        gctx->alteredPC = true;
    }

    /**
     * @brief Generates the next-state relation for the AllocaInst @c I
     * This allocates memory on the heap
     *
     * Changes:
     *   - ["processes"][gctx->thread_id]["r"]
     *   - ["processes"][gctx->thread_id]["pc"]
     *   - ["processes"][gctx->thread_id]["memory"]
     *   - ["processes"][gctx->thread_id]["memory_info"]
     *
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    void generateNextStateForInstruction(GenerationContext* gctx, AllocaInst* I) {
        assert(I);

        // Download the heap
        Heap heap(gctx, gctx->svout);
        heap.download();

        // Perform the malloc
        auto ptr = heap.malloc(I);

        // Store the pointer in the model-register
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* ret = vReg(registers, I);
        ptr = gctx->gen->builder.CreateIntToPtr(ptr, I->getType());
        gctx->gen->builder.CreateStore(ptr, ret);

        // Debug
        builder.CreateCall(pins("printf"), {generateGlobalString("stored %i to %p\n"), ptr, ret});

        // Upload the heap
        heap.upload();

        // Action label
        std::string str;
        raw_string_ostream strout(str);
        strout << "r[" << valueRegisterIndex[I] << "] " << *I;

        ChunkMapper cm_action(*this, gctx->model, type_action);
        Value* actionLabelChunkID = cm_action.generatePut( ConstantInt::get(t_int, strout.str().length())
                                                         , generateGlobalString(strout.str())
                                                         );
        gctx->edgeLabels["action"] = actionLabelChunkID;
    }

    /**
     * @brief Generates the next-state relation for the Load or Store @c I
     *
     * Changes:
     *   - ["processes"][gctx->thread_id]["r"]
     *   - ["processes"][gctx->thread_id]["memory"]
     *
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    template<typename T>
    void generateNextStateForMemoryInstruction(GenerationContext* gctx, T* I) {
        assert(I);
        Value* ptr = I->getPointerOperand();

        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

        // Load the memory by making a copy
        ChunkMapper cm_memory(*this, gctx->model, type_memory);
        auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
        Value* sv_memorylen;
        auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);
        builder.CreateCall(pins("printf"), {generateGlobalString("Allocated %i bytes\n"), sv_memorylen});

        // Clone the memory instruction
        Instruction* IC = I->clone();
        IC->setName("IC");

        // Value-map all the operands of the instruction
        for (auto& OI: IC->operands()) {

            // If this is the pointer operand to a global, a different mapping
            // needs be performed
            if(dyn_cast<Value>(OI.get()) == ptr && !dyn_cast<GlobalVariable>(ptr)) {

                // Load the model-register to obtain the pointer to memory
                OI = builder.CreateLoad(vReg(registers, OI.get()));

                // Since the pointer is an offset within the heap, add
                // the start address of the loaded memory chunk to the
                // offset to obtain the current real address. Current
                // means within the scope of this next-state call.
                OI = builder.CreatePtrToInt(OI, t_intptr);
                OI = builder.CreateAdd(OI, builder.CreatePtrToInt(sv_memorydata, t_intptr));
                OI = builder.CreateIntToPtr(OI, I->getPointerOperand()->getType());

                // Debug
                // FIXME: add out-of-bounds check
                builder.CreateCall(pins("printf"), {generateGlobalString("access @ %x\n"), OI});

            // If this is a 'normal' register, simply map it
            } else {
                OI = vMap(gctx, OI);
            }

        }

        // Insert the cloned instruction
        builder.Insert(IC);

        // If the instruction has a return value, store the result in the SV
        if(IC->getType() != t_void) {
            if(valueRegisterIndex.count(I) == 0) {
                roout << *I << " (" << I << ")" << "\n";
                roout.flush();
                assert(0 && "instruction not indexed");
            }
            builder.CreateStore(IC, vReg(registers, I));
        }

        // Upload the new memory chunk
        auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
        builder.CreateStore(newMem, chunkMemory);
    }

    /**
     * @brief Generates the next-state relation for the CallInst @c I
     *
     * Note that this uses the PC of gctx->svout to determine where to
     * jump back to after the function finishes. It changes the PC
     * to the start of the called function.
     *
     * Changes:
     *   - ["processes"][gctx->thread_id]["pc"]
     *   - ["processes"][gctx->thread_id]["stk"]
     *
     * @param gctx
     * @param I
     */
    void generateNextStateForCall(GenerationContext* gctx, CallInst* I) {
        assert(I);

        // If this is a declaration, there is no body within the LLVM model,
        // thus we need to handle it differently.
        Function* F = I->getCalledFunction();
        if(F->isDeclaration()) {

            auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            auto it = gctx->gen->hookedFunctions.find(F->getName());

            // If this is a hooked function
            if(it != gctx->gen->hookedFunctions.end()) {

                // Map the operands
                std::vector<Value*> args;
                for(unsigned int i = 0; i < I->getNumArgOperands(); ++i) {
                    auto Arg = I->getArgOperand(i);

                    // Leave constants as they are
                    if(dyn_cast<Constant>(Arg)) {
                        args.push_back(Arg);

                    // If we have a known mapping, perform the mapping
                    } else if(valueRegisterIndex.count(Arg) > 0) {
                        Arg = builder.CreateLoad(vReg(registers, Arg));

                    // Otherwise, report an error
                    } else {
                        roout << "Could not find mapping for " << *Arg << "\n";
                        roout.flush();
                    }
                }

                gctx->gen->builder.CreateCall(it->second, {});
            }

        // Otherwise, there is an LLVM body available, so it is modeled.
        } else {
            std::vector<Value*> args;
            for(int i=0; i < I->getNumArgOperands(); ++i) {
                args.push_back(I->getArgOperand(i));
            }
            stack.pushStackFrame(gctx, *F, args);
            gctx->alteredPC = true;
        }
    }

    void generateNextStateForInstruction(GenerationContext* gctx, BranchInst* I) {
        auto dst_pc = lts["processes"][0]["pc"].getValue(gctx->svout);

        // If this branch instruction has a condition
        if(I->isConditional()) {

            Value* condition = I->getCondition();
            assert(condition);
            if(I->getNumSuccessors() == 2) {

                // Set the program counter to either the true basic block or
                // the false basic block. This is done by a Select instruction,
                // which does basically:
                //   loc = cond ? locTrue : locFalse;
                // at run-time.
                auto locTrue = gctx->gen->programLocations[&*I->getSuccessor(0)->begin()];
                auto locFalse = gctx->gen->programLocations[&*I->getSuccessor(1)->begin()];
                assert(locTrue);
                assert(locFalse);
                Value* newLoc = builder.CreateSelect( vMap(gctx, condition)
                                                    , ConstantInt::get(gctx->gen->t_int, locTrue)
                                                    , ConstantInt::get(gctx->gen->t_int, locFalse)
                                                    );
                gctx->gen->builder.CreateStore(newLoc, dst_pc);
            } else {
                roout << "Conditional Branch has more or less than 2 successors: " << *I << "\n";
                roout.flush();
            }
        } else {
            if(I->getNumSuccessors() == 1) {
                auto loc = gctx->gen->programLocations[&*I->getSuccessor(0)->begin()];
                assert(loc);
                gctx->gen->builder.CreateStore(ConstantInt::get(gctx->gen->t_int, loc), dst_pc);
            } else {
                roout << "Unconditional branch has more or less than 1 successor: " << *I << "\n";
                roout.flush();
            }
        }
    }


    /**
     * @brief generates the pop interface to LTSmin
     */
    void generateInterface() {

        // The name of the plugin
        auto name = ConstantDataArray::getString(ctx, "llmc");
        new GlobalVariable( *pinsModule
                          , name->getType()
                          , true
                          , GlobalValue::ExternalLinkage
                          , name
                          , "pins_plugin_name"
                          );

        // Loader record
        Type* t_loaderrecord = StructType::create( ctx
                                                 , {t_voidp, t_voidp}
                                                 , "t_loaderrecord"
                                                 );
        new GlobalVariable( *pinsModule
                          , t_loaderrecord
                          , true
                          , GlobalValue::ExternalLinkage
                          , ConstantAggregateZero::get(t_loaderrecord)
                          , "pins_loaders"
                          );

        // popt options
        auto t_poptoption = up_pinsHeaderModule->getTypeByName("poptOption");
        assert(t_poptoption);
        new GlobalVariable( *pinsModule
                          , t_poptoption
                          , true
                          , GlobalValue::ExternalLinkage
                          , ConstantAggregateZero::get(t_poptoption)
                          , "poptOptions"
                          );

        // Generate the popt callback function
        // void poptCallback( poptContext con, enum poptCallbackReason reason
        //                  , const struct poptOption * opt, const char * arg, void * data
        //                  )
        auto t_poptcallback = FunctionType::get( t_void
                                               , { t_voidp
                                                 , t_int
                                                 , PointerType::get(t_poptoption, 0)
                                                 , t_charp
                                                 , t_voidp
                                                 }
                                               , false
                                               );

        auto f_poptcallback = Function::Create( t_poptcallback
                                              , GlobalValue::LinkageTypes::ExternalLinkage
                                              , "poptCallback"
                                              , pinsModule
                                              );
        { // poptCallback

            auto args = f_poptcallback->arg_begin();
            Argument* con = &*args++;
            Argument* reason = &*args++;
            Argument* opt = &*args++;
            Argument* arg = &*args++;
            Argument* data = &*args++;

            (void)con;
            (void)opt;
            (void)arg;
            (void)data;

            BasicBlock* entry  = BasicBlock::Create(ctx, "entry" , f_poptcallback);
            BasicBlock* pre    = BasicBlock::Create(ctx, "pre" , f_poptcallback);
            BasicBlock* post   = BasicBlock::Create(ctx, "post" , f_poptcallback);
            BasicBlock* option = BasicBlock::Create(ctx, "option" , f_poptcallback);
            BasicBlock* def    = BasicBlock::Create(ctx, "def" , f_poptcallback);

            builder.SetInsertPoint(entry);
            SwitchInst* sw = builder.CreateSwitch(reason, def, 3);
            sw->addCase(ConstantInt::get(t_int, POPT_CALLBACK_REASON_PRE), pre);
            sw->addCase(ConstantInt::get(t_int, POPT_CALLBACK_REASON_POST), post);
            sw->addCase(ConstantInt::get(t_int, POPT_CALLBACK_REASON_OPTION), option);

            builder.SetInsertPoint(pre);
            builder.CreateRetVoid();

            builder.SetInsertPoint(post);
            builder.CreateRetVoid();

            builder.SetInsertPoint(option);
            builder.CreateRetVoid();

            builder.SetInsertPoint(def);
            builder.CreateRetVoid();

        }

        // Generate the model initialization function
        // void pins_model_init(model_t)
        t_model = pinsModule->getTypeByName("grey_box_model");
        t_modelp = t_model->getPointerTo(0);
        auto t_pins_model_init = FunctionType::get(t_void, t_modelp, false);
        auto f_pins_model_init = Function::Create( t_pins_model_init
                                                 , GlobalValue::LinkageTypes::ExternalLinkage
                                                 , "pins_model_init", pinsModule
                                                 );
        { // pins_model_init

            // Get the argument (the model)
            auto args = f_pins_model_init->arg_begin();
            Argument* model = &*args++;

            // Create some space for chunks
            BasicBlock* entry  = BasicBlock::Create(ctx, "entry", f_pins_model_init);
            builder.SetInsertPoint(entry);
            auto sv_memory_init_info = builder.CreateAlloca(t_chunkid);
            auto sv_memory_init_data = builder.CreateAlloca(t_chunkid);

            // Determine the size of the dependency matrix
            auto t_matrix = pinsModule->getTypeByName("matrix");
            auto t_matrixp = t_matrix->getPointerTo(0);
            auto t_statevector_count = ConstantExpr::getUDiv( t_statevector_size
                                                            , ConstantInt::get(t_int, 4, true)
                                                            );
            auto t_matrix_size = builder.CreateIntCast( generateSizeOf(t_matrix)
                                                      , pins("malloc")->getFunctionType()->getParamType(0)
                                                      , false
                                                      );

            // Create the dependency matrix
            Value* dm = builder.CreateCall(pins("malloc"), t_matrix_size);
            dm = builder.CreateBitOrPointerCast(dm, t_matrixp);
            builder.CreateCall( pins("dm_create")
                              , { dm
                                , ConstantInt::get(t_int, transitionGroups.size())
                                , t_statevector_count
                                }
                              );

            // Fill the DM with '1's
            // FIXME: this can be optimized
            builder.CreateCall(pins("dm_fill"), {dm});

            // Generate the LTS type
            Value* ltstype = generateLTSType();

            // Set the LTS and DM
            builder.CreateCall(pins("GBsetLTStype"), {model, ltstype});
            builder.CreateCall(pins("GBsetDMInfo"), {model, dm});

            // Initialize the initial state
            {
                // Init to 0
                builder.CreateMemSet( s_statevector
                                    , ConstantInt::get(t_int8, 0)
                                    , t_statevector_size
                                    , s_statevector->getPointerAlignment(pinsModule->getDataLayout())
                                    );

                // Init the memory layout
                auto F = llmc_func("llmc_os_memory_init");
                builder.CreateCall( F
                                  , { builder.CreatePointerCast(model, F->getFunctionType()->getParamType(0))
                                    , ConstantInt::get(t_int, type_memory->index)
                                    , sv_memory_init_info
                                    , ConstantInt::get(t_int, type_memory_info->index)
                                    , sv_memory_init_data
                                    }
                                  );

                // llmc_os_memory_init stores the chunkIDs of the memory and
                // memory_info chunks in the allocas passed to it, so load
                // and store them in the initial state
                auto sv_memory_init_info_l = builder.CreateLoad(sv_memory_init_info, "sv_memory_init_info");
                auto sv_memory_init_data_l = builder.CreateLoad(sv_memory_init_data, "sv_memory_init_data");
                builder.CreateStore( sv_memory_init_info_l
                                   , lts["processes"][0]["memory_info"].getValue(s_statevector)
                                   );
                builder.CreateStore( sv_memory_init_data_l
                                   , lts["processes"][0]["memory"].getValue(s_statevector)
                                   );

                // Make sure the empty stack is at chunkID 0
                ChunkMapper cm_stack(*this, model, type_stack);
                cm_stack.generatePutAt(ConstantInt::get(t_int, 0), ConstantPointerNull::get(t_intp), 0);

                // Set the initialization value for the globals
                for(auto& v: module->getGlobalList()) {
                    if(v.hasInitializer()) {
                        int idx = valueRegisterIndex[&v];
                        auto& node = lts["globals"][idx];
                        assert(node.getChildren().size() == 0);
                        builder.CreateStore(v.getInitializer(), node.getValue(s_statevector));
                    }
                }
            }

            // Set the initial state
            auto initStateType = pins("GBsetInitialState")->getFunctionType()->getParamType(1);
            auto initState = builder.CreatePointerCast(s_statevector, initStateType);
            builder.CreateCall(pins("GBsetInitialState"), {model, initState});

            // Set the next-state function
            auto t_next_method_grey = pins_type("next_method_grey_t");
            assert(t_next_method_grey);
            builder.CreateCall( pins("GBsetNextStateLong")
                              , {model, builder.CreatePointerCast(f_pins_getnext, t_next_method_grey)}
                              );
            builder.CreateRetVoid();
        }

    }

    /**
      * @brief Generates the lts_*() calls for the specified SVTree nodes
      * @param ltstype The Value for the ltstype pointer known by LTSmin
      * @param pinsTypes A mapping from type name to LTSmin type ID
      * @param F The function the calls will be generated in
      * @param leavesSV Vector of nodes to generate calls for
      * @param typeno_setter The name of the function to set the typeno
      * @param name_setter The name of the function to set the name
      *
      * Flattens the SVTree node structure. This is done by generating calls
      * to lts_*() functions for every leaf in the tree. For every leaf, a
      * for-loop is generated that loops until the claimed number of slots
      * are enough to fit the type that the leaf specifies.
      */
    void generateLTSTypeCallsForSVTree( Value* ltstype
                                      , std::unordered_map<std::string, Value*>& pinsTypes
                                      , Function* F
                                      , SVTree& root
                                      , std::string typeno_setter
                                      , std::string name_setter
                                      ) {

        std::vector<SVTree*> leavesSV;
        root.executeLeaves([this, &leavesSV](SVTree* node) {
            leavesSV.push_back(node);
        });

        // Create a memory location the count the slot index. Start at 0.
        auto slot = addAlloca(t_int, F);
        builder.CreateStore(ConstantInt::get(t_int, 0), slot);

        // Remember the previous block, so we know where we came from and
        // we can add it to the PHI node later
        BasicBlock* prev = builder.GetInsertBlock();

        // The for_end of the previous for-loop will be the for_cond of
        // the current for-loop
        BasicBlock* for_end = BasicBlock::Create(ctx, "for_cond" , F);
        builder.CreateBr(for_end);
        builder.SetInsertPoint(for_end);

        // Generate the for-loop for each leaf
        for(SVTree* node: leavesSV) {

            // Skip empty leaves
            if(node->isEmpty()) {
                continue;
            }

            // Get the size of the variable
            Constant* varsize = node->getSizeValueInSlots();
            ConstantInt* varsize_int = dyn_cast<ConstantInt>(varsize);

            // Create BasicBlocks
            BasicBlock* forcond = for_end;
            BasicBlock* forbody = BasicBlock::Create(ctx, "for_body_" + node->getName(), F);
            BasicBlock* forincr = BasicBlock::Create(ctx, "for_incr" , F);
            for_end = BasicBlock::Create(ctx, "for_end_cond" , F);

            // Create the condition
            PHINode* tg = builder.CreatePHI(t_int, 2, "TG");
            Value* cond = builder.CreateICmpULT(tg, varsize);
            builder.CreateCondBr(cond, forbody, for_end);

            // Load the slot index
            builder.SetInsertPoint(forbody);
            Value* slotv = builder.CreateLoad(t_int, slot);

            // If the variable needs multiple slots, the name will have a
            // counter, counting the number of slots from the start of the
            // variable in the state-vector
            IRStringBuilder irs(*this, 256);
            irs << node->getFullName();
            irs << tg;
            auto name = irs.str();

            name = builder.CreateSelect( builder.CreateICmpEQ(varsize, ConstantInt::get(t_int, 1))
                                       , generateGlobalString(node->getFullName())
                                       , name
                                       );

            builder.CreateCall( pins("printf")
                              , { generateGlobalString("Slot %i (%i/%i): type %i, name %s\n")
                                , slotv
                                , tg
                                , varsize
                                , pinsTypes[node->getType()->_name]
                                , name
                                }
                              );

            // Get the LTSmin type index of the leaf
            auto v_typeno = pinsTypes[node->getType()->_name];
            if(!v_typeno) {
                roout << "Error: type was not added to LTS: " << node->getType()->_name << "\n";
                roout.flush();
                exit(-1);
            }

            // Set the type and name of this slot
            builder.CreateCall(pins(typeno_setter), {ltstype, slotv, v_typeno});
            builder.CreateCall(pins(name_setter), {ltstype, slotv, name});

            // Increment the slot index
            slotv = builder.CreateAdd(slotv, ConstantInt::get(t_int, 1));
            builder.CreateStore(slotv, slot);

            // Branch to the branch that increments the tg index
            builder.CreateBr(forincr);

            // Increment the tg index and branch to the condition
            builder.SetInsertPoint(forincr);
            Value* nextTG = builder.CreateAdd(tg, ConstantInt::get(t_int, 1));
            builder.CreateBr(forcond);

            // Tell the Phi node of the incoming edges
            tg->addIncoming(ConstantInt::get(t_int, 0), prev);
            tg->addIncoming(nextTG, forincr);

            // Set the previous
            prev = forcond;

            // The for_end will be the for_cond of the next iteration,
            // so set the insertion point to it
            builder.SetInsertPoint(for_end);
        }

        Value* slotv = builder.CreateLoad(t_int, slot);
        auto root_size = generateSizeOf(root.getLLVMType());
        auto root_count = ConstantExpr::getUDiv(root_size, ConstantInt::get(t_int, 4));
        builder.CreateCall( pins("printf")
                          , { generateGlobalString("Populated %i / %i slots\n")
                            , slotv
                            , root_count
                            }
                          );
    }

    /**
     * @brief Generates the LTS type for LTSmin of the pins model. This is done
     * by generating code using the current state of the builder. This should
     * be done in pins_model_init()
     * @return The LTS type
     */
    Value* generateLTSType() {

        Function* F = builder.GetInsertBlock()->getParent();

        // Create the LTS type
        Value* ltstype = builder.CreateCall(pins("lts_type_create"));
        auto t_statevector_count = ConstantExpr::getUDiv(t_statevector_size, ConstantInt::get(t_int, 4));
        builder.CreateCall(pins("lts_type_set_state_length"), {ltstype, t_statevector_count});

        // Type of the third argument of lts_type_add_type()
        auto p = dyn_cast<PointerType>(pins("lts_type_add_type")->getFunctionType()->getParamType(2));

        // gather all the leaves
        std::unordered_map<std::string, Value*> pinsTypes;
        std::vector<SVTree*> leavesSV;
        std::vector<SVTree*> leavesLabels;
        std::vector<SVTree*> leavesEdgeLabels;
        lts.getSV().executeLeaves([this, &leavesSV](SVTree* node) {
            leavesSV.push_back(node);
        });
        if(lts.hasLabels()) {
            lts.getLabels().executeLeaves([this, &leavesLabels](SVTree* node) {
                leavesLabels.push_back(node);
            });
            if( leavesLabels.size() == 1
             && leavesLabels[0] == &lts.getLabels()
             && leavesLabels[0]->getType() == nullptr
              ) {
                leavesLabels.clear();
            }
        }
        if(lts.hasEdgeLabels()) {
            lts.getEdgeLabels().executeLeaves([this, &leavesEdgeLabels](SVTree* node) {
                leavesEdgeLabels.push_back(node);
            });
            if( leavesEdgeLabels.size() == 1
             && leavesEdgeLabels[0] == &lts.getEdgeLabels()
             && leavesEdgeLabels[0]->getType() == nullptr
              ) {
                leavesEdgeLabels.clear();
            }
        }

        // Loop over the types in the LTS to make them known to LTSmin
        for(auto& nameType: lts.getTypes()) {
            auto nameV = generateGlobalString(nameType->_name);
            auto& t = pinsTypes[nameType->_name];
            t = builder.CreateCall(pins("lts_type_add_type"), {ltstype, nameV, ConstantPointerNull::get(p)});
            builder.CreateCall( pins("lts_type_set_format")
                              , {ltstype, t, ConstantInt::get(t_int, nameType->_ltsminType)}
                              );
            builder.CreateCall(pins("printf"), {generateGlobalString("Added type %i: %s\n"), t, nameV});
        }

        builder.CreateCall( pins("printf")
                          , { generateGlobalString("t_statevector_size = %i\n")
                            , t_statevector_size
                            }
                          );
        builder.CreateCall( pins("printf")
                          , { generateGlobalString("t_statevector_count = %i\n")
                            , t_statevector_count
                            }
                          );

        // Generate the lts_*() calls to set the types for the state vector
        generateLTSTypeCallsForSVTree( ltstype
                                     , pinsTypes
                                     , F
                                     , lts.getSV()
                                     , "lts_type_set_state_typeno"
                                     , "lts_type_set_state_name"
                                     );

        // Generate the lts_*() calls to set the types for the edge labels
        builder.CreateCall( pins("lts_type_set_edge_label_count")
                          , { ltstype
                            , ConstantInt::get(t_int, lts.getEdgeLabels().count())
                            }
                          );
        if(lts.hasEdgeLabels()) {
            generateLTSTypeCallsForSVTree( ltstype
                                         , pinsTypes
                                         , F
                                         , lts.getEdgeLabels()
                                         , "lts_type_set_edge_label_typeno"
                                         , "lts_type_set_edge_label_name"
                                         );
        }

        // FIXME: add state label support
        assert(leavesLabels.size() == 0);

        return ltstype;
    }

    /**
     * @brief Obtain the LLVM Function for the function name @c name
     * @param name The name of the LLVM Function to return
     * @return The LLVM Function named @c name
     *
     * This is suited for LLMC OS functions
     *
     */
    Function* llmc_func(std::string name) const {
        auto f = pinsModule->getFunction(name);
        assert(f);
        return f;
    }

    /**
     * @brief Obtain the LLVM Function for the function name @c name
     * @param name The name of the LLVM Function to return
     * @return The LLVM Function named @c name
     *
     * This is suited for PINS functions
     *
     */
    Function* pins(std::string name) const {
        auto f = pinsModule->getFunction(name);
        assert(f);
        return f;
    }

    /**
     * @brief Obtain the LLVM Type for the type name @c name
     * @param name The name of the LLVM Type to return
     * @return The LLVM Type named @c name
     */
    Type* pins_type(std::string name) const {

        // Get the type from the PINS header module
        Type* type = up_pinsHeaderModule->getTypeByName(name);

        // If not found, check if there is an alias for it
        if(!type) {
            for(auto& a: up_pinsHeaderModule->getAliasList()) {
                if(a.getName().str() == name) {
                    PointerType* p = a.getType();
                    Type* t = p->getElementType();
                    type = t;
                }
            }
        }

        // Return the type
        assert(type);
        return type;
    }

    /**
     * @brief Adds an AllocaInst for type @c type in the entry block of
     * function @c f.
     * @param type The Type to allocate
     * @param f The function in which entry block to allocate
     * @param size The number of instances of @c type to allocate
     * @return The generated AllocaInst
     */
    static AllocaInst* addAlloca(Type* type, Function* f, Value* size = nullptr) {
        IRBuilder<> b(type->getContext());
        b.SetInsertPoint(&*f->getEntryBlock().begin());
        auto v = b.CreateAlloca(type, size);
        return v;
    }

    /**
     * @brief Prints the PINS module LLVM IR to the file @c s.
     * @param s The file to write to.
     */
    void writeTo(std::string s) {
        std::error_code EC;
        raw_fd_ostream fdout(s, EC, sys::fs::OpenFlags::F_RW);
        pinsModule->print(fdout, nullptr);
    }

    /**
     * @brief Generates a global const char[] for the string @c s.
     * @param s The string to generate a global const char[] for.
     * @return Pointer const char* to the generated const char[]
     */
    Value* generateGlobalString(std::string s) {
        auto& var = generatedStrings[s];
        auto t = ArrayType::get(t_int8, s.length()+1);
        if(var == nullptr) {
            var = new GlobalVariable( *pinsModule
                                    , t
                                    , true
                                    , GlobalValue::LinkageTypes::ExternalLinkage
                                    , ConstantDataArray::getString(ctx, s)
                                    );
        }
        Value* indices[2];
        indices[0] = (ConstantInt::get(t_int, 0, true));
        indices[1] = (ConstantInt::get(t_int, 0, true));
        return builder.getFolder().CreateGetElementPtr(t, var, indices);
    }

};

} // namespace llmc
