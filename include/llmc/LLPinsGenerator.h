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
extern "C" {
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>
}

#include <llmc/generation/ChunkMapper.h>
#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>
#include <llmc/generation/ProcessStack.h>
#include <llmc/generation/TransitionGroups.h>

#include "llvmgen.h"

extern "C" {
#include "popt.h"
}

using namespace llvm;

namespace llmc {

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
        ChunkMapper cm_memory;
    public:

        Heap(GenerationContext* gctx, Value* sv)
        :   gctx(gctx)
        ,   sv(sv)
        ,   memory(nullptr)
        ,   memory_len(nullptr)
        ,   memory_data(nullptr)
        ,   cm_memory(gctx, gctx->gen->type_memory)
        {
            //assert(gctx->thread_id==0 && "this code assumes all heap vars to be in the same pool");
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

            auto memory = cm_memory.generateGet(svMemory);
            memory_data      = gctx->gen->generateChunkGetData(memory);
            memory_len       = gctx->gen->generateChunkGetLen(memory);
//            auto v_memory_data      = gctx->gen->generateChunkGetData(memory);
//            auto v_memory_len       = gctx->gen->generateChunkGetLen(memory);

//            auto F = gctx->gen->builder.GetInsertBlock()->getParent();
//            if(!memory_data)      memory_data      = addAlloca(v_memory_data->getType()     , F);
//            if(!memory_len)       memory_len       = addAlloca(v_memory_len->getType()      , F);
//
//            gctx->gen->builder.CreateStore(v_memory_data, memory_data);
//            gctx->gen->builder.CreateStore(v_memory_len , memory_len);
        }

        /**
         * @brief Allocate @c n bytes in the memory-chunk.
         * @param n The number of bytes to allocate
         * @return Value Pointer to the newly allocated memory
         */
        Value* malloc(Value* n) {

            //download();

            llvmgen::BBComment(gctx->gen->builder, "Heap: malloc");

            auto svMemory = gctx->gen->lts["processes"][gctx->thread_id]["memory"].getValue(sv);

            Value* oldLength;
            memory_data = cm_memory.generateGetAndCopy(svMemory, oldLength, n);
            memory_len = gctx->gen->builder.CreateAdd(oldLength, n);

            return gctx->gen->builder.CreateCall(gctx->gen->llmc_func("llmc_os_memory_malloc"), {memory_data, n});
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
         */
        void upload() {

            llvmgen::BBComment(gctx->gen->builder, "Heap: upload");

            auto svMemory = gctx->gen->lts["processes"][gctx->thread_id]["memory"].getValue(sv);

//            auto v_memory_data = gctx->gen->builder.CreateLoad(memory_data, "memory_data");
//            auto v_memory_len = gctx->gen->builder.CreateLoad(memory_len, "memory_len");
            auto memory = cm_memory.generatePut(memory_len, memory_data);
            gctx->gen->builder.CreateStore(memory, svMemory);
        }

    };

public:
    std::unique_ptr<llvm::Module> up_module;
    LLVMContext& ctx;
    llvm::Module* module;
    llvm::Module* pinsModule;
    std::unique_ptr<llvm::Module> up_pinsHeaderModule;
    std::unique_ptr<llvm::Module> up_libllmcosModule;
    std::vector<TransitionGroup*> transitionGroups;
    llvm::Type* t_void;
    llvm::Type* t_voidp;
    llvm::Type* t_voidpp;
    llvm::Type* t_charp;
    llvm::IntegerType* t_int;
    llvm::PointerType* t_intp;
    llvm::IntegerType* t_intptr;
    llvm::IntegerType* t_int8;
    llvm::IntegerType* t_int64;
    llvm::PointerType* t_int64p;
    llvm::StructType* t_chunk;
    llvm::IntegerType* t_pthread_t;
    llvm::Type* t_chunkid;
    llvm::StructType* t_model;
    llvm::PointerType* t_modelp;
    llvm::ArrayType* t_registers_max;
    Value* t_registers_max_size;
    llvm::StructType* t_statevector;
    llvm::StructType* t_edgeLabels;
    llvm::FunctionType* t_pins_getnext;
    llvm::FunctionType* t_pins_getnextall;

    static int const MAX_THREADS = 2;
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

    LLVMLTSType lts;

    SVType* type_status;
    SVType* type_pc;
    SVType* type_tid;
    SVType* type_threadresults;
    SVType* type_stack;
    SVType* type_register_frame;
    SVType* type_registers;
    SVType* type_memory;
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
        , stack(this)
        , debugChecks(false)
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
        tries.emplace_back(File(System::getBinaryLocation() + "/../libllmc", name));

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
                out.reportError("Error loading module " + name + ": " + moduleFile.getFilePath() + ":");
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
            return module;
        } else {
            out.reportError( "Error loading module "
                           + name
                           + ", file does not exist: `"
                           + moduleFile.getFilePath()
                           + "'"
                           );
            for(auto& f: tries) {
                if(f.exists()) {
                    out.reportNote("Tried: " + f.getFilePath());
                }
            }
            assert(0 && "oh noes");
            return std::unique_ptr<Module>(nullptr);
        }

    }

    /**
     * @brief
     */
     void enableDebugChecks() {
         debugChecks = true;
     }

    /**
     * @brief Starts the pinsification process.
     */
    bool pinsify() {
        bool ok = true;

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

        // Create the register mapping used to map registers to locations
        // in the state-vector
        createRegisterMapping();

        // Generate the LLVM types we need
        generateTypes();

        // Generate the LLVM functions that we will populate later
        generateSkeleton();

        // Initialize the stack helper functions
        stack.init();

        // Generate the initial state
        generateInitialState();

        // Generate the contents of the next state function
        // for...
//        for(int i = 0; i < MAX_THREADS; ++i) {
//            generateNextState(i);
//        }
        generateNextState();

        // Generate Popt interfacee
        generateInterface();

        generateDebugInfo();


        //pinsModule->dump();

        out.reportAction("Verifying module...");
        out.indent();

        if(verifyModule(*pinsModule, &roout)) {
            out.reportFailure("Generated LLVM module verification failed");
            ok = false;
        } else {
            out.reportSuccess("Module OK");
        }
        out.outdent();

        return ok;
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
      DISubprogram *getnextall;
      DISubprogram *moduleinit;
      DIType *DblTy;
      std::vector<DIScope *> LexicalBlocks;

      DIType *getDoubleTy();
    };

    /**
     * @brief Attach debug info to all generated instructions.
     */
    void generateDebugInfo() {
        pinsModule->addModuleFlag(Module::Warning, "Dwarf Version", DEBUG_METADATA_VERSION);
        pinsModule->addModuleFlag(Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);
        pinsModule->addModuleFlag(Module::Max, "PIC Level", 2);

        DIBuilder& dbuilder = *getDebugBuilder();

        DebugInfo dbgInfo;
        dbgInfo.TheCU = getCompileUnit();
        assert(dbgInfo.TheCU);
        assert(dbgInfo.TheCU->getFile());

        dbgInfo.getnext = getDebugScopeForFunction(f_pins_getnext);

        dbgInfo.moduleinit = getDebugScopeForFunction(pinsModule->getFunction("pins_model_init"));

        dbuilder.finalize();

        int l = 1;
        for(auto& F: *pinsModule) {
//            out.reportNote("Debug info for " + F.getName().str());
//            out.indent();
            DISubprogram* dip = F.getSubprogram();
            if(!dip) {
//                out.reportNote("Skipping");
//                out.outdent();
                continue;
            }
            for(auto& BB: F) {
                for(Instruction& I: BB) {
                    if(!I.getDebugLoc()) {
                        I.setDebugLoc(DILocation::get(ctx, l, 1, dip, nullptr));
                    }
                    l++;
                }
            }
//            out.outdent();
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

    Value* generatePointerAdd(Value* ptr, Value* intToAdd) {
        auto ptr_type = ptr->getType();
        ptr = builder.CreatePtrToInt(ptr, t_intptr);
        intToAdd = builder.CreateIntCast( intToAdd, t_intptr, false);
        ptr = builder.CreateAdd(ptr, intToAdd);
        ptr = builder.CreateIntToPtr(ptr, ptr_type);
        return ptr;
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

            builder.CreateCall(pins("printf"), {generateGlobalString("[%2i] trying transition group\n"), grp});


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
            gctx.cb = cb;
            gctx.userContext = user_context;
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
            builder.CreateCall(pins("printf"), {generateGlobalString("[%2i] no\n"), grp});
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
        t_int64p = PointerType::get(t_int64, 0);
        t_intptr = IntegerType::get(ctx, 64); // FIXME
        t_void = PointerType::getVoidTy(ctx);
        t_charp = PointerType::get(IntegerType::get(ctx, 8), 0);
        t_voidp = t_charp; //PointerType::get(PointerType::getVoidTy(ctx), 0);
        t_voidpp = PointerType::get(t_voidp, 0);

        // Chunk types
        t_chunkid = IntegerType::getInt32Ty(ctx);
        t_chunk = dyn_cast<StructType>(pins("pins_chunk_get")->getReturnType());

        t_pthread_t = IntegerType::get(ctx, 64);

        // Determine the register size
        out.reportAction("Determining register size in state vector");
        out.indent();
        auto& DL = pinsModule->getDataLayout();
        size_t registersInBytes = 0;
        for(auto& kv: registerLayout) {
            if(!kv.second.registerLayout->isSized()) {
                roout << "Type is not sized: " << *kv.second.registerLayout << "\n";
                roout.flush();
                assert(0);
            }
            auto bytesNeeded = DL.getTypeSizeInBits(kv.second.registerLayout);
            std::stringstream ss;
            ss << kv.first->getName().str() << " needs " << (bytesNeeded/8) << " bytes";
            out.reportNote(ss.str());
            if(registersInBytes < bytesNeeded) {
                registersInBytes = bytesNeeded;
            }
        }
        out.outdent();
        t_registers_max = ArrayType::get(t_int, registersInBytes/32); // needs to be based on max #registers of all functions
        t_registers_max_size = builder.CreateGEP( t_registers_max
                                                , ConstantPointerNull::get(t_registers_max->getPointerTo())
                                                , {ConstantInt::get(t_int, 1)}
                                                );
        std::stringstream ss;
        ss << "Determined required number of registers: " << (registersInBytes/32) << " slots";
        out.reportAction(ss.str());

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
        type_tid = new SVType( "tid"
                                 , LTStypeSInt32
                                 , t_int
                                 , this
                                 );
        type_threadresults = new SVType( "threadresults"
                                 , LTStypeChunk
                                 , t_chunkid
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
        lts << type_tid;
        lts << type_threadresults;
        lts << type_pc;
        lts << type_stack;
        lts << type_register_frame;
        lts << type_registers;
        lts << type_memory;
        lts << type_heap;
        lts << type_action;

        // Create the state-vector tree of variable nodes
        auto sv = new SVRoot(this, "type_sv");

        // Processes
        auto sv_processes = new SVTree("processes", "processes");
        for(int i=0; i < MAX_THREADS; ++i) {
            std::stringstream ss;
            ss << "proc" << i;
            auto sv_proc = new SVTree(ss.str(), "process");
            *sv_proc
                << new SVTree("status", type_status)
                << new SVTree("tid", type_tid)
                << new SVTree("pc", type_pc)
                << new SVTree("memory", type_memory)
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
            << new SVTree("threadsStarted", type_tid)
            << new SVTree("tres", type_threadresults)
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
    void generateNextState() {

        // Get the arguments
        auto args = f_pins_getnext->arg_begin();
        Argument* self = &*args++;
        Argument* grp = &*args++;
        Argument* src = &*args++;
        Argument* cb = &*args++;
        Argument* user_context = &*args++;

        (void)self;
        (void)grp;
        (void)cb;
        (void)user_context;

        // Make a copy of the generation context
        GenerationContext gctx_copy = generationContexts["pins_getnext"];
        GenerationContext* gctx = &gctx_copy;

        assert(src == gctx->src);


        // For all transitions groups
        for(auto& t: transitionGroups) {

            // Reset the generation context
            gctx->edgeLabels.clear();
            gctx->alteredPC = false;

            // If the transition group (TG) models an instruction
            if(t->getType() == TransitionGroup::Type::Instructions) {
                auto ti = static_cast<TransitionGroupInstructions*>(t);

                // Set the thread ID
                gctx->thread_id = ti->thread_id;

                // Set the insertion point to the point in the switch
                builder.SetInsertPoint(ti->nextState);

                // Create BBs
                auto bb_transition = BasicBlock::Create(ctx, "bb_transition", f_pins_getnext);
                auto bb_end = BasicBlock::Create(ctx, "bb_end", f_pins_getnext);

                // Access the PCs
                auto src_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->src);
                auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);

                // Check that the PC in the SV matches the PC of this TG
                auto pc = builder.CreateLoad(src_pc, "pc");
                auto pc_check = builder.CreateICmpEQ( pc
                                                    , ConstantInt::get(t_int, programLocations[ti->instructions.front()])
                                                    );
                builder.CreateCondBr(pc_check, bb_transition, bb_end);

                // If the PC is a match
                {
                    builder.SetInsertPoint(bb_transition);
                    assert(t_statevector_size);

                    // Copy the SV into a local one
                    auto cpy = builder.CreateMemCpy( gctx->svout
                                                   , gctx->svout->getPointerAlignment(pinsModule->getDataLayout())
                                                   , gctx->src
                                                   , src->getParamAlignment()
                                                   , t_statevector_size
                                                   );
                    setDebugLocation(cpy, __FILE__, __LINE__);

                    // Update the destination PC
                    builder.CreateStore(ConstantInt::get(t_int, programLocations[ti->instructions.back()->getNextNode()]), dst_pc);

                    // Debug
                    std::string str;
                    raw_string_ostream strout(str);

                    //builder.CreateCall(pins("printf"), {generateGlobalString("Instruction: " + strout.str() + "\n")});

                    // Generate the next state for the instructions
                    for(auto I: ti->instructions) {
                        strout << "; ";
                        if(valueRegisterIndex.count(I)) {
                            strout << "r[" << valueRegisterIndex[I] << "] ";
                        }
                        strout << " {";
                        strout << I->getFunction()->getName();
                        strout << "} ";
                        strout << *I;
                        generateNextStateForInstruction(gctx, I);
                    }

                    // Generate the next state for all other actions
                    for(auto& action: ti->actions) {
                        generateNextStateForInstruction(gctx, action);
                    }

                    // Generate the action label
                    llvmgen::BBComment(builder, "Instruction: " + strout.str());
                    ChunkMapper cm_action(gctx, type_action);
                    Value* actionLabelChunkID = cm_action.generatePut( ConstantInt::get(t_int, strout.str().length())
                            , generateGlobalString(strout.str())
                    );
                    gctx->edgeLabels["action"] = actionLabelChunkID;

                    // DEBUG: print labels
                    for(auto& nameValue: gctx->edgeLabels) {
                        auto value = lts.getEdgeLabels()[nameValue.first].getValue(gctx->edgeLabelValue);
                        builder.CreateStore(nameValue.second, value);
//                        builder.CreateCall( pins("printf")
//                                          , { generateGlobalString("label: %i at %p\n")
//                                            , nameValue.second
//                                            , value
//                                            }
//                                          );
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
//                    std::string s = "[%2i] transition (pc:%i) %3i -> %3i [" + t->desc + "] (%p)\n";
//                    builder.CreateCall( pins("printf")
//                                      , { generateGlobalString(s)
//                                        , grp
//                                        , pc
//                                        , ConstantInt::get(t_int, programLocations[ti->srcPC])
//                                        , ConstantInt::get(t_int, programLocations[ti->dstPC])
//                                        , edgelabels
//                                        }
//                                      );
                    builder.CreateBr(f_pins_getnext_end_report);
                }

                // If the PC is not a match
                {
                    builder.SetInsertPoint(bb_end);
//                    builder.CreateCall( pins("printf")
//                                      , { generateGlobalString("no: %i %i\n")
//                                        , pc
//                                        , ConstantInt::get(t_int, programLocations[ti->srcPC])
//                                        }
//                                      );
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

            GenerationContext gctx_copy = *gctx;
            gctx_copy.thread_id = 0;

            // Generate the check that all threads are inactive and main()
            // has not been called before
            Value* cond = lts["status"].getValue(gctx_copy.src);
            cond = builder.CreateLoad(t_int, cond, "status");
            cond = builder.CreateICmpEQ(cond, ConstantInt::get(t_int, 0));
            for(int i = 0; i < MAX_THREADS; ++i) {
                auto src_pc = lts["processes"][i]["pc"].getValue(gctx_copy.src);
                auto pc = builder.CreateLoad(t_int, src_pc, "pc");
                auto cond2 = builder.CreateICmpEQ(pc, ConstantInt::get(t_int, 0));
                cond = builder.CreateAnd(cond, cond2);
            }

            BasicBlock* main_start  = BasicBlock::Create(ctx, "main_start" , f_pins_getnext);
            builder.CreateCondBr(cond, main_start, f_pins_getnext_end);

            builder.SetInsertPoint(main_start);
            builder.CreateCall(pins("printf"), {generateGlobalString("main_start\n")});
            auto cpy = builder.CreateMemCpy(gctx_copy.svout, gctx_copy.svout->getPointerAlignment(pinsModule->getDataLayout()), src, src->getParamAlignment(), t_statevector_size);
            setDebugLocation(cpy, __FILE__, __LINE__);
//            for(int i = 0; i < MAX_THREADS; ++i) {
//                gctx_copy.thread_id = i;
                stack.pushStackFrame(&gctx_copy, *module->getFunction("main"), {}, nullptr);
//            }
            //builder.CreateStore(ConstantInt::get(t_int, 1), svout_pc[0]);
            builder.CreateStore(ConstantInt::get(t_int, 1), lts["status"].getValue(gctx_copy.svout));
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
        BasicBlock::iterator II = BB.begin();
        BasicBlock::iterator IE = BB.end();
        while(II != IE) {
            II = createTransitionGroupsForInstruction(II, IE);
        }
    }

    bool isCollapsableInstruction(Instruction* I) {
        switch(I->getOpcode()) {
            case Instruction::Alloca:
                return true;
            case Instruction::Load:
            case Instruction::Store:
                return false;
            case Instruction::Ret:
            case Instruction::Br:
            case Instruction::Switch:
            case Instruction::IndirectBr:
            case Instruction::Invoke:
            case Instruction::Resume:
                return false;
            case Instruction::Unreachable:
                return true;
            case Instruction::CleanupRet:
            case Instruction::CatchRet:
            case Instruction::CatchSwitch:
                return false;
            case Instruction::GetElementPtr:
                return true;
            case Instruction::Fence:
            case Instruction::AtomicCmpXchg:
            case Instruction::AtomicRMW:
                return false;
            case Instruction::CleanupPad:
            case Instruction::PHI:
                return true;
            case Instruction::Call:
                if(dyn_cast<CallInst>(I)->getIntrinsicID() != Intrinsic::not_intrinsic) {
                    return true;
                }
                return false;
            case Instruction::VAArg:
            case Instruction::ExtractElement:
            case Instruction::InsertElement:
            case Instruction::ShuffleVector:
            case Instruction::ExtractValue:
            case Instruction::InsertValue:
                return true;
            case Instruction::LandingPad:
                return false;
            default:
                return true;
        }
    }

    /**
     * @brief Create transition groups for the Instruction @c I
     * @param I The LLVM Instruction to create transition groups for
     */
    BasicBlock::iterator createTransitionGroupsForInstruction(BasicBlock::iterator& It, BasicBlock::iterator& IE) {

        std::vector<Instruction*> instructions;

        // Generate a description
        std::string desc;
        raw_string_ostream rdesc(desc);

        while(It != IE && isCollapsableInstruction(&*It)) {

            auto I = &*It;
            rdesc << I;

            // Assign a PC to the instruction if it does not have one yet
            if(programLocations[I] == 0) {
                programLocations[I] = nextProgramLocation++;
            }

            instructions.push_back(I);

            It++;
        }

        do {

            auto I = &*It;
            rdesc << I;

            // Assign a PC to the instruction if it does not have one yet
            if(programLocations[I] == 0) {
                programLocations[I] = nextProgramLocation++;
            }

            instructions.push_back(I);

            It++;
        } while(It != IE && isCollapsableInstruction(&*It));

        if(programLocations[&*It] == 0) {
            programLocations[&*It] = nextProgramLocation++;
        }

        // Create the TG
        for(int i = 0; i < MAX_THREADS; ++i) {
            auto tg = new TransitionGroupInstructions(i, instructions, {}, {});
            tg->setDesc(rdesc.str());
            addTransitionGroup(tg);
        }

        // Print
        out << "  " << programLocations[instructions.front()] << " -> " << programLocations[instructions.back()] << " ";
        //I.print(roout, true);
        out << std::endl;

        return It;
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
        out.reportError("Value not an instruction or argument: ");
        roout << *reg->getType();
        assert(0);
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

    Value* vRegUsingOffset(Value* registers, Value* offsetInBytes, Type* type) {


        /// pointer add!
        auto v = generatePointerAdd(registers, offsetInBytes);

        v = builder.CreatePointerCast(v, type->getPointerTo());

        return v;
    }

    /**
     * @brief This makes sure that the specified program-Value is mapped
     * to a model-Value in a register. As source-Value can be specified
     * any of the following:
     *   - GlobalVariable
     *   - ConstantExpr
     *   - Constant
     *   - any mapped value in valueRegisterIndex
     * In the case of a mapped value, the value is loaded from the register
     * in the state-vector.
     * @param gctx
     * @param OI
     * @return
     */
    Value* vMap(GenerationContext* gctx, Value* OI) {

        assert(OI);

        // Map globals
        if(isa<GlobalVariable>(OI)) {
            auto idx = valueRegisterIndex[OI];
            return lts["globals"][idx].getValue(gctx->svout);

        // Change ConstantExprs to an instruction, because further down they
        // could reference a global. Normally a global is a constant pointer,
        // but our globals are in the state vector, meaning they are not
        // really globals, so they are not constant. Thus these need to be
        // changed to instructions.
        } else if(auto ce = dyn_cast<ConstantExpr>(OI)) {
            auto result = ce->getAsInstruction();
            vMapOperands(gctx, result);
            builder.Insert(result);
            return result;

        // Leave constants as they are
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
        setDebugLocation(IC, __FILE__, __LINE__);

        // If the instruction has a return value, store the result in the SV
        if(IC->getType() != t_void) {
            assert(valueRegisterIndex[I]);
            auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            auto store = builder.CreateStore(IC, vReg(registers, I));
            setDebugLocation(store, __FILE__, __LINE__);
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
                assert(0);
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
        stack.popStackFrame(gctx, I);
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
     *
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    void generateNextStateForInstruction(GenerationContext* gctx, AllocaInst* I) {
        assert(I);

        Heap heap(gctx, gctx->svout);

        // Perform the malloc
        auto ptr = heap.malloc(I);
        heap.upload();

        // Store the pointer in the model-register
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* ret = vReg(registers, I);
        ptr = gctx->gen->builder.CreateIntToPtr(ptr, I->getType());
        auto store = gctx->gen->builder.CreateStore(ptr, ret);
        setDebugLocation(store, __FILE__, __LINE__);

        // Debug
        builder.CreateCall(pins("printf"), {generateGlobalString("stored %i to %p\n"), ptr, ret});

        // Action label
        std::string str;
        raw_string_ostream strout(str);
        strout << "r[" << valueRegisterIndex[I] << "] " << *I;

        ChunkMapper cm_action(gctx, type_action);
        Value* actionLabelChunkID = cm_action.generatePut( ConstantInt::get(t_int, strout.str().length())
                                                         , generateGlobalString(strout.str())
                                                         );
        gctx->edgeLabels["action"] = actionLabelChunkID;

// attempt to report only one transition
//        auto r = builder.CreateCall(f_pins_getnextall, {gctx->model, gctx->svout, gctx->cb, gctx->userContext});
//        builder.CreateRet(r);
//        BasicBlock* main_start  = BasicBlock::Create(ctx, "unreachable" , builder.GetInsertBlock()->getParent());
//        builder.SetInsertPoint(main_start);

    }

    DIBuilder* getDebugBuilder() {
        static DIBuilder* dib = nullptr;
        if(!dib) {
            dib = new DIBuilder(*pinsModule);
        }
        return dib;
    }

    DICompileUnit* getCompileUnit() {
        static DICompileUnit* cu = nullptr;
        if(!cu) {
#if LLVM_VERSION_MAJOR < 4
            cu = getDebugBuilder()->createCompileUnit( dwarf::DW_LANG_hi_user
                                                      , pinsModule->getName(), ".", "llmc", false, "", 0);
#else
            auto file = getDebugBuilder()->createFile(pinsModule->getName(), ".");
            cu = getDebugBuilder()->createCompileUnit(dwarf::DW_LANG_C, file, "llmc", 0, "", 0);
#endif
        }
        return cu;
    }

    DIScope* getDebugScopeForSourceFile(DIScope* functionScope, std::string const& fileName) {
        return functionScope;
        auto file = getDebugBuilder()->createFile(pinsModule->getName(), ".");
        return DILexicalBlockFile::get(functionScope->getContext(), functionScope, file, 0);
    }

    DISubprogram* getDebugScopeForFunction(Function* F) {
        static std::unordered_map<Function*, DISubprogram*> scopes;
        DISubprogram*& scope = scopes[F];
        if(!scope) {
            auto a = getDebugBuilder()->getOrCreateTypeArray({});
            auto d_getnext = getDebugBuilder()->createSubroutineType(a);
            scope = getDebugBuilder()->createFunction( getCompileUnit()->getFile()
                                                     , F->getName()
                                                     , ""
                                                     , getCompileUnit()->getFile()
                                                     , 0
                                                     , d_getnext
                                                     , 0
                                                     , DINode::FlagPrototyped
                                                     , DISubprogram::SPFlagDefinition
                                                     );
            F->setSubprogram(scope);
        }
        return scope;
    }

    void setDebugLocation(Instruction* I, std::string const& file, int linenr) {
        //auto f = getDebugBuilder()->createFile(file, ".");
        I->setDebugLoc(DebugLoc::get( linenr
                                    , 0
                                    , getDebugScopeForSourceFile(getDebugScopeForFunction(I->getParent()->getParent()), file)
        ));
    }

    void setDebugLocation(Value* V, std::string const& file, int linenr) {
        Instruction* I = dyn_cast<Instruction>(V);
        if(I) {
            setDebugLocation(I, file, linenr);
        }
    }

    Value* getChunkPartOfPointer(Value* v) {
        v = builder.CreatePtrToInt(v, t_intptr);
        v = builder.CreateLShr(v, 32);
        return v;
    }

    Value* getOffsetPartOfPointer(Value* v) {
        v = builder.CreatePtrToInt(v, t_intptr);
        v = builder.CreateBinOp(Instruction::BinaryOps::And, v, ConstantInt::get(t_intptr, 0xFFFFFFFFULL, false));
        return v;
    }

    void generateNextStateForStoreInstruction(GenerationContext* gctx, StoreInst* I) {
        Value* ptr = I->getPointerOperand();

        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

        if(isa<GlobalVariable>(ptr) || isa<ConstantExpr>(ptr) || isa<Constant>(ptr)) {
            Instruction* IC = I->clone();
            IC->setName("IC");
            vMapOperands(gctx, IC);
            builder.Insert(IC);
            setDebugLocation(IC, __FILE__, __LINE__);
        } else {
            ChunkMapper cm_memory(gctx, type_memory);
            auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
            Value* sv_memorylen = nullptr;

            //Value* chunkID = getChunkPartOfPointer(ptr);
            ptr = builder.CreateLoad(vReg(registers, ptr));
            Value* offset = getOffsetPartOfPointer(ptr);
            auto val = I->getValueOperand();
            if(isa<Argument>(val) || isa<Instruction>(val)) {
                val = vReg(registers, val);
            } else {
                auto valMapped = vMap(gctx, val);
                //val = builder.CreateAlloca(val->getType());
                auto alloc = addAlloca(val->getType(), builder.GetInsertBlock()->getParent());
                val = alloc;
                setDebugLocation(alloc, __FILE__, __LINE__);
                auto store = builder.CreateStore(valMapped, val);
                setDebugLocation(store, __FILE__, __LINE__);
            }
            auto newMem = cm_memory.generateCloneAndModify(chunkMemory, offset, val, generateSizeOf(I->getValueOperand()), sv_memorylen);
            builder.CreateStore(newMem, chunkMemory);
        }
    }

    void generateNextStateForLoadInstruction(GenerationContext* gctx, LoadInst* I) {
    }

    Value* vMapPointer(Value* registers, Value* sv_memorydata, Value* modelPointerValue, Type* type) {
        assert(!isa<ConstantExpr>(modelPointerValue) && "this is meant for pointers into memory");
        assert(!isa<Constant>(modelPointerValue) && "this is meant for pointers into memory");

        // Load the model-register to obtain the pointer to memory
        modelPointerValue = builder.CreateLoad(vReg(registers, modelPointerValue));

        // Since the pointer is an offset within the heap, add
        // the start address of the loaded memory chunk to the
        // offset to obtain the current real address. Current
        // means within the scope of this next-state call.
        modelPointerValue = builder.CreatePtrToInt(modelPointerValue, t_intptr);
        modelPointerValue = builder.CreateAdd(modelPointerValue, builder.CreatePtrToInt(sv_memorydata, t_intptr));
        modelPointerValue = builder.CreateIntToPtr(modelPointerValue, type);

        // Debug
        // FIXME: add out-of-bounds check
//                builder.CreateCall(pins("printf"), {generateGlobalString("access @ %x\n"), OI});
        return modelPointerValue;
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
        ChunkMapper cm_memory(gctx, type_memory);
        auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
        Value* sv_memorylen;
        auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);
//        builder.CreateCall(pins("printf"), {generateGlobalString("Allocated %i bytes\n"), sv_memorylen});

        // Clone the memory instruction
        Instruction* IC = I->clone();
        IC->setName("IC");

        // Value-map all the operands of the instruction
        for (auto& OI: IC->operands()) {

            // If this is the pointer operand to a global, a different mapping
            // needs be performed
            if(dyn_cast<Value>(OI.get()) == ptr && !dyn_cast<GlobalVariable>(ptr)) {

                OI = vMapPointer(registers, sv_memorydata, OI.get(), I->getPointerOperand()->getType());

            // If this is a 'normal' register or global, simply map it
            } else {
                OI = vMap(gctx, OI);
            }

        }

        // Insert the cloned instruction
        builder.Insert(IC);
        setDebugLocation(IC, __FILE__, __LINE__);

        // If the instruction has a return value, store the result in the SV
        if(IC->getType() != t_void) {
            if(valueRegisterIndex.count(I) == 0) {
                roout << *I << " (" << I << ")" << "\n";
                roout.flush();
                assert(0 && "instruction not indexed");
            }
            builder.CreateStore(IC, vReg(registers, I));
//            builder.CreateCall( pins("printf")
//                              , {generateGlobalString("Setting register %i to %i")
//                                , ConstantInt::get(t_int, valueRegisterIndex[I])
//                                , IC
//                                }
//                              );
        }

//        if(debugChecks) {
//            if(IC->getOpcode() == Instruction::Store) {
//                // Make sure register remain the same
//                auto registers_src = lts["processes"][gctx->thread_id]["r"].getValue(gctx->src);
//                createMemAssert(registers_src, registers, t_registers_max_size);
//            } else if(IC->getOpcode() == Instruction::Load) {
//                // Make sure globals are the same
//                auto globals = lts["globals"].getValue(gctx->svout);
//                auto globals_src = lts["globals"].getValue(gctx->src);
//                createMemAssert(globals_src, globals, generateSizeOf(lts["globals"].getLLVMType()));
//            } else {
//                assert(0);
//            }
//        }

        // Upload the new memory chunk
        auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
        builder.CreateStore(newMem, chunkMemory);
    }

    void createMemAssert(Value* a, Value* b, Value* size) {
        a = builder.CreatePointerCast(a, t_voidp);
        b = builder.CreatePointerCast(b, t_voidp);
        size = builder.CreateIntCast(size, t_int64, false);
        Instruction* cmp = builder.CreateCall(pins("memcmp"), {a, b, size});
        setDebugLocation(cmp, __FILE__, __LINE__);

        llvmgen::If If(builder, "if_memory_assert");
        If.setCond(builder.CreateICmpNE(cmp, ConstantInt::get(t_int, 0)));
        BasicBlock* BBTrue = If.getTrue();
        If.generate();

        builder.SetInsertPoint(&*BBTrue->getFirstInsertionPt());
        auto call = builder.CreateCall( pins("printf")
                          , {generateGlobalString("Memory assertion failed")
                            }
                          );
        setDebugLocation(call, __FILE__, __LINE__);
        call = builder.CreateCall(pins("exit"), {ConstantInt::get(t_int, 1)});
        setDebugLocation(call, __FILE__, __LINE__);
        builder.SetInsertPoint(If.getFinal());
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

        // If this is inline assembly
        if(I->isInlineAsm()) {
            auto inlineAsm = dyn_cast<InlineAsm>(I->getCalledValue());
            out.reportNote("Found ASM: " + inlineAsm->getAsmString());
            assert(0);
        } else {

            // If this is a declaration, there is no body within the LLVM model,
            // thus we need to handle it differently.
            Function* F = I->getCalledFunction();
            if(!F) {
                roout << "Function call calls null: " << *I << "\n";
                roout.flush();
                abort();
            }
            if(F->isDeclaration()) {

                auto it = gctx->gen->hookedFunctions.find(F->getName());

                // If this is a hooked function
                if(it != gctx->gen->hookedFunctions.end()) {

                    // Print
                    out.reportNote("Handling " + F->getName().str());

                    assert(&I->getContext() == &F->getContext());

                    // Map the operands
                    std::vector<Value*> args;
                    for(unsigned int i = 0; i < I->getNumArgOperands(); ++i) {
                        auto Arg = I->getArgOperand(i);

                        Arg = vMap(gctx, Arg);
                        args.push_back(Arg);
                    }
                    auto call = gctx->gen->builder.CreateCall(it->second, args);
                    setDebugLocation(call, __FILE__, __LINE__);

                } else if(F->isIntrinsic()) {
                } else if(F->getName().equals("pthread_create")) {
                    // int pthread_create( pthread_t *thread
                    //                   , const pthread_attr_t *attr
                    //                   , void *(*start_routine) (void *)
                    //                   , void *arg);
                    //auto F = pins("pthread_create");
                    //assert(F);

                    auto BBEnd = BasicBlock::Create(ctx, "pthread_create_if_pos_end", builder.GetInsertBlock()->getParent());
                        auto threadsStarted_p = lts["threadsStarted"].getValue(gctx->svout);

                    // For every thread position, generate code that checks if it is
                    // free. If so, put the new thread there. Else, continue.
                    for(int tIdx = 0; tIdx < MAX_THREADS; ++tIdx) {

                        // Check that PC == 0
                        auto pc = lts["processes"][tIdx]["pc"].getValue(gctx->svout);
                        pc = builder.CreateLoad(pc);
                        auto cmp = builder.CreateICmpEQ(pc, ConstantInt::get(t_int, 0));

                        // Setup if
                        stringstream ss;
                        ss << "pthread_create_if_pos_" << tIdx << "_is_free";
                        llvmgen::If2 genIf(builder, cmp, ss.str(), BBEnd);

                        // If that PC == 0, push stack frame
                        genIf.startTrue();

                        auto threadFunction = dyn_cast<Function>(I->getArgOperand(2));
                        assert(threadFunction);

                        GenerationContext gctx_copy = *gctx;
                        gctx_copy.thread_id = tIdx;
                        stack.pushStackFrame(&gctx_copy, *threadFunction, {I->getArgOperand(3)}, nullptr);

                        // Update thread ID and threadsStarted
                        auto tid_p = lts["processes"][tIdx]["tid"].getValue(gctx->svout);
                        Value* threadsStarted = builder.CreateLoad(threadsStarted_p);
                        threadsStarted = builder.CreateAdd(threadsStarted, ConstantInt::get(t_int, 1));
                        builder.CreateStore(threadsStarted, tid_p);
                        builder.CreateStore(threadsStarted, threadsStarted_p);

                        // Load the memory by making a copy
                        ChunkMapper cm_memory(gctx, type_memory);
                        auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
                        Value* sv_memorylen;
                        auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);

                        auto registers = lts["processes"][tIdx]["r"].getValue(gctx->svout);
                        auto tid_in_program = vMapPointer(registers, sv_memorydata, I->getArgOperand(0), t_int64p);
//                        auto tid_in_program = vReg(registers, *I->getParent()->getParent(), "tid", I->getArgOperand(0));
//                        tid_in_program = builder.CreatePointerBitCastOrAddrSpaceCast(tid_in_program, t_int64p);
                        builder.CreateStore( builder.CreateIntCast(threadsStarted, t_int64, false)
                                           , tid_in_program
                                           );
                        builder.CreateCall( pins("printf")
                                          , { generateGlobalString("Storing %zu to pthread_t %p %p\n")
                                            , builder.CreateIntCast(threadsStarted, t_int64, false)
                                            , tid_in_program
                                            , registers
                                            }
                                          );

                        // Upload the new memory chunk
                        auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
                        builder.CreateStore(newMem, chunkMemory);

                        genIf.endTrue();

                        // If it is false, do nothing, the next iteration will
                        // generate the next if
                        genIf.startFalse();

                    }

                    // For the last false-case, go to BBEnd
                    builder.CreateCall( pins("printf")
                                      , { generateGlobalString("ERROR: COULD NOT SPAWN MORE THREADS\n")
                                        }
                                      );
                    auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);
                    builder.CreateStore(ConstantInt::get(t_int, programLocations[I]), dst_pc);
                    builder.CreateBr(BBEnd);

                    builder.SetInsertPoint(BBEnd);
                } else if(F->getName().equals("pthread_join")) {
                    // int pthread_join(pthread_t thread, void **value_ptr);
                    //auto F = pins("pthread_join");
                    //assert(F);

                    //auto BBEnd = BasicBlock::Create(ctx, "pthread_join_if_pos_end", builder.GetInsertBlock()->getParent());

                    ChunkMapper cm_tres = ChunkMapper(gctx, type_threadresults);

                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
                    auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);
                    auto tres_p = lts["tres"].getValue(gctx->svout);
                    auto results = cm_tres.generateGet(tres_p);
                    auto results_data = generateChunkGetData(results);
                    auto results_len = generateChunkGetLen(results);

                    // Setup if
                    //llvmgen::For genFor(builder, "pthread_join_result_loop");

                    // Set condition
                    //genFor.setRange(results_data, generatePointerAdd(results_data, results_len), ConstantInt::get(t_int, 8));

    //                auto result = builder.CreateAlloca(t_int64);
                    auto tid = vReg(registers, *I->getParent()->getParent(), "tid", I->getArgOperand(0));
                    tid = builder.CreatePointerBitCastOrAddrSpaceCast(tid, t_int64p);
                    tid = builder.CreateLoad(tid, t_intp);
                    //tid = builder.CreatePtrToInt(tid, t_int);

                    auto storeResult = vReg(registers, *I->getParent()->getParent(), "pthread_join_result", I);
                    storeResult = builder.CreatePointerCast(storeResult, t_voidpp);

                    auto llmc_list_find = pins("llmc_list_find");
                    auto found = builder.CreateCall( llmc_list_find
                                                   , { results_data
                                                     , results_len
                                                     , tid
                                                     , storeResult
                                                     }
                                                   );

                    llvmgen::If genIf(builder, "pthread_join_result_not_found");
                    genIf.setCond(builder.CreateICmpEQ(found, ConstantInt::get(found->getType(), 0)));
                    auto BBTrue = genIf.getTrue();
                    genIf.generate();

                    builder.SetInsertPoint(&*BBTrue->getFirstInsertionPt());
                    builder.CreateStore(ConstantInt::get(t_int, programLocations[I]), dst_pc);

                    builder.SetInsertPoint(genIf.getFinal());

    //                type_threadresults

                    // True case
    //                auto BBTrue = genIf.getTrue();
    //                builder.SetInsertPoint(BBTrue);
    //
    //                auto writeThreadResultTo = I->getArgOperand(1);
    //                cm_tres ChunkMapper(*this, gctx->
    //
    //                // After the true case happened, go to BBEnd
    //                genIf.setFinal(BBEnd);
    //
    //                // If it is false, do nothing, the next iteration will
    //                // generate the next if
    //                auto BBFalse = genIf.getTrue();
    //
    //                genIf.generate();
    //                builder.SetInsertPoint(BBFalse);

                } else {
                    out.reportError("Unhandled function call: " + F->getName().str());
                }

            // Otherwise, there is an LLVM body available, so it is modeled.
            } else {
                std::vector<Value*> args;
                for(unsigned int i=0; i < I->getNumArgOperands(); ++i) {
                    args.push_back(I->getArgOperand(i));
                }
                stack.pushStackFrame(gctx, *F, args, I);
            }
            gctx->alteredPC = true;
        }
    }

    void generateNextStateForInstruction(GenerationContext* gctx, BranchInst* I) {
        auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);
        auto dst_reg = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

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
                auto s = gctx->gen->builder.CreateStore(ConstantInt::get(gctx->gen->t_int, loc), dst_pc);
                setDebugLocation(s, __FILE__, __LINE__);
            } else {
                roout << "Unconditional branch has more or less than 1 successor: " << *I << "\n";
                roout.flush();
            }
        }

        // If the target has PHI nodes, go through them to determine the value
        // for the PHI node associated with the origin-branch
        for(unsigned int succ = 0; succ < I->getNumSuccessors(); ++succ) {
            for(auto& phiNode: I->getSuccessor(succ)->phis()) {
                // Assign the value to the PHI node
                // associated with the origin-branch
                auto c = builder.CreateStore(vMap(gctx, phiNode.getIncomingValueForBlock(I->getParent())), vReg(dst_reg, &phiNode));
                setDebugLocation(c, __FILE__, __LINE__);
            }
        }

    }

    void printLocation(std::string const& file, size_t line) {
        builder.CreateCall( pins("printf")
                          , { generateGlobalString("now at %s:%u\n")
                            , generateGlobalString(file)
                            , ConstantInt::get(t_int, line)
                            }
                          );
        fflush(stdout);
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

                GenerationContext gctx = {};
                gctx.gen = this;
                gctx.model = model;
                gctx.src = nullptr;
                gctx.svout = s_statevector;
                gctx.alteredPC = false;

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
                                    , sv_memory_init_data
                                    }
                                  );

                // llmc_os_memory_init stores the chunkIDs of the memory chunk
                // in the allocas passed to it, so load and store them in the
                // initial state
                auto sv_memory_init_data_l = builder.CreateLoad(sv_memory_init_data, "sv_memory_init_data");
                for(size_t threadID = 0; threadID < MAX_THREADS; ++threadID) {
                    builder.CreateStore( sv_memory_init_data_l
                                       , lts["processes"][threadID]["memory"].getValue(s_statevector)
                                       );
                }

                // Init stack
                for(size_t threadID = 0; threadID < MAX_THREADS; ++threadID) {
                    auto pStackChunkID = lts["processes"][threadID]["stk"].getValue(s_statevector);
                    builder.CreateStore(stack.getEmptyStack(&gctx), pStackChunkID);
                }

                ChunkMapper cm_tres = ChunkMapper(&gctx, type_threadresults);
                auto tres_p = lts["tres"].getValue(s_statevector);
                auto t_tres = StructType::get(ctx, {t_int64, t_voidp});
                auto newTres = builder.CreateAlloca(t_tres);
                auto len = generateSizeOf(t_tres);
                builder.CreateMemSet( newTres
                                        , ConstantInt::get(t_int8, 0)
                                        , len
                                        , 1
                                        );

                auto newTresChunk = cm_tres.generatePut(len, newTres);
                builder.CreateStore(newTresChunk, tres_p);

                // Make sure the empty stack is at chunkID 0
//                ChunkMapper cm_stack(*this, model, type_stack);
//                cm_stack.generatePutAt(ConstantInt::get(t_int, 4), ConstantPointerNull::get(t_intp), 0);

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
            llvmgen::IRStringBuilder<LLPinsGenerator> irs(*this, 256);
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
        IRBuilder<> b(f->getContext());
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
        raw_fd_ostream fdout(s, EC, sys::fs::FA_Read | sys::fs::FA_Write);
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
