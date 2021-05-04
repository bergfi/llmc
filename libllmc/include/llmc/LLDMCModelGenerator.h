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

#include <llmc/generation/StateManager.h>
#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>
#include <llmc/generation/ProcessStack.h>
#include <llmc/generation/TransitionGroups.h>

#include "llvmgen.h"

using namespace llvm;
using namespace libfrugi;

namespace llmc {

/**
 * @class FunctionData
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLDMCModelGenerator.h
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
 * @class LLDMCModelGenerator
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLDMCModelGenerator.h
 * @brief The class that does the work
 */
class LLDMCModelGenerator {
public:

    enum GlobalStatus: int32_t {
        START,
        RUNNING_CONSTRUCTORS,
        STARTING_MAIN,
        RUNNING_MAIN,
        ENDED,
        ENDED_FAILURE,
    };

    friend class ProcessStack;

public:
    std::unique_ptr<llvm::Module> up_module;
    LLVMContext& ctx;
    llvm::Module* module;
    llvm::Module* dmcModule;
//    std::unique_ptr<llvm::Module> up_pinsHeaderModule;
    std::unique_ptr<llvm::Module> up_dmcapiHeaderModule;
    std::unique_ptr<llvm::Module> up_libllmcosModule;
    std::unique_ptr<llvm::Module> up_libllmcvmModule;
    std::vector<TransitionGroup*> transitionGroups;
    llvm::Type* t_void;
    llvm::PointerType* t_voidp;
    llvm::PointerType* t_voidpp;
    llvm::PointerType* t_charp;
    llvm::IntegerType* t_char;
    llvm::IntegerType* t_bool;
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
    llvm::StructType* t_globals;
    llvm::StructType* t_edgeLabels;
    llvm::FunctionType* t_dmc_nextstates;
    llvm::FunctionType* t_dmc_initialstate;
    llvm::FunctionType* t_globalStep;
    llvm::FunctionType* t_stepProcess;

    llvm::Constant* ptr_offset_threadid;
    llvm::Constant* ptr_mask_threadid;
    llvm::Constant* ptr_offset_location;
    llvm::Constant* ptr_mask_location;

    static int const MAX_THREADS = 6;
    static int const BITWIDTH_STATEVAR = 32;
    llvm::Constant* c_bytewidth_statevar;
    static int const BITWIDTH_INT = 32;
    llvm::Constant* t_statevector_size;

    GlobalVariable* s_statevector;
    llvm::Constant* c_globalMemStart;

    llvm::Function* f_stepProcess;
    llvm::Function* f_dmc_nextstates;
    llvm::Function* f_dmc_initialstate;
    llvm::Function* f_constructorStart;
    llvm::Function* f_mainStart;

    llvm::Function* f_dmc_insert;
    llvm::Function* f_dmc_insertBytes;
    llvm::Function* f_dmc_transition;
    llvm::Function* f_dmc_get;
    llvm::Function* f_dmc_delta;
    llvm::Function* f_dmc_deltaBytes;
    llvm::Function* f_dmc_getpart;
    llvm::Function* f_dmc_getpartBytes;

    llvm::BasicBlock* f_pins_getnext_end;
    llvm::BasicBlock* f_pins_getnext_end_report;

    llvm::IRBuilder<> builder;
    llvm::ConstantFolder folder;

    std::unordered_map<Instruction*, int> programLocations;
    std::unordered_map<Value*, int> valueRegisterIndex;
    std::unordered_map<GlobalVariable*, GlobalVariable*> mappedConstantGlobalVariables;
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
    SVType* type_size;
//    SVType* type_heap;
    SVType* type_action;

    SVTree* root_threadresults;
    SVTree* root_threadresults_element;
    SVType* type_threadresults_element;

    ProcessStack stack;

//    std::unordered_map<std::string, Function*> hookedFunctions;

    bool debugChecks;
    bool _assumeNonAtomicCollapsable;
    SVTypeManager typeManager;

public:
    LLDMCModelGenerator(std::unique_ptr<llvm::Module> modul, MessageFormatter& out)
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
        , _assumeNonAtomicCollapsable(false)
        , typeManager(this)
        {
        module = up_module.get();
        dmcModule = nullptr;
    }

    ~LLDMCModelGenerator() {
        for(auto& tg: transitionGroups) {
            delete tg;
        }
    }

    /**
     * @brief Cleans up this instance.
     */
    void cleanup() {
        if(dmcModule) {
            delete dmcModule;
        }
        dmcModule = nullptr;
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

     void assumeNonAtomicCollapsable() {
         _assumeNonAtomicCollapsable = true;
     }

    /**
     * @brief Starts the pinsification process.
     */
    bool pinsify() {
        bool ok = true;

        cleanup();

        dmcModule = new Module("dmcModule", ctx);

        dmcModule->setDataLayout("e-p:64:64:64-i1:32:32-i8:32:32-i16:32:32-i32:32:32-i64:64:64-i128:128:128-f16:32:32-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a:0:64");
        dmcModule->addModuleFlag(Module::Warning, "Dwarf Version", DEBUG_METADATA_VERSION);
        dmcModule->addModuleFlag(Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);
        dmcModule->addModuleFlag(Module::Max, "PIC Level", 2);

        out.reportAction("Loading internal modules...");
        out.indent();
        up_libllmcosModule = loadModule("libllmcos.ll");
        up_libllmcvmModule = loadModule("libllmcvm.ll");
//        up_pinsHeaderModule = loadModule("pins.h.ll");
        up_dmcapiHeaderModule = loadModule("libdmccapi.ll");
        out.outdent();

//        out.reportAction("Linking in LLMCOS");
//        {
//            out.indent();
//            llvm::Linker linker(*module);
//            if(linker.linkInModule(std::move(up_libllmcosModule), Linker::Flags::None)) {
//                out.reportError("failed");
//            }
//            out.outdent();
//        }

        // Copy function declarations from the loaded modules into the
        // module of the model we are now generating such that we can call
        // these functions
//        out.reportAction("Adding PINS API");
//        for(auto& f: up_pinsHeaderModule->getFunctionList()) {
// //           if(!f.getName().startswith("pins_") && !f.getName().startswith("dmc_")) continue;
//            if(!dmcModule->getFunction(f.getName().str())) {
//                Function::Create(f.getFunctionType(), f.getLinkage(), f.getName(), dmcModule);
//                out.reportAction2(f.getName().str());
//            }
//        }
        out.reportAction("Adding DMC API");
//        for(auto& f: up_dmcapiHeaderModule->getFunctionList()) {
//            if(!dmcModule->getFunction(f.getName().str())) {
//                Function::Create(f.getFunctionType(), GlobalValue::LinkageTypes::ExternalLinkage, f.getName(), dmcModule);
//                out.reportAction2(f.getName().str());
//            }
//        }
//        {
//            out.indent();
//            llvm::Linker linker(*dmcModule);
//            if(linker.linkInModule(std::move(up_dmcapiHeaderModule), Linker::Flags::None)) {
//                out.reportError("failed");
//            }
//            out.outdent();
//        }
        f_dmc_insert = dmcapi_inject_func("dmc_insert");
        f_dmc_insertBytes = dmcapi_inject_func("dmc_insertB");
        f_dmc_transition = dmcapi_inject_func("dmc_transition");
        f_dmc_get = dmcapi_inject_func("dmc_get");
        f_dmc_delta = dmcapi_inject_func("dmc_delta");
        f_dmc_deltaBytes = dmcapi_inject_func("dmc_deltaB");
        f_dmc_getpart = dmcapi_inject_func("dmc_getpart");
        f_dmc_getpartBytes = dmcapi_inject_func("dmc_getpartB");

        out.reportAction("Setting up LLMC OS VM Hooks");
//        out.indent();
        for(auto& f: up_libllmcvmModule->getFunctionList()) {
            auto const& functionName = f.getName().str();
            if(!dmcModule->getFunction(functionName)) {
                Function::Create(f.getFunctionType(), f.getLinkage(), f.getName(), dmcModule);
//                out.reportNote(f.getName().str());
//                if(!functionName.compare(0, 10, "llmc_hook_")) {
//                    std::string name = functionName.substr(10);
//                    hookedFunctions[name] = F;
//                    out.reportNote("Hooking " + name + " to " + functionName);
//                }
            }
        }
//        out.outdent();

        // Check if PINS API available
//        bool pinsapiok = true;
//        for(auto& func: {"pins_chunk_put64", "pins_chunk_cam64", "pins_chunk_get64", "pins_chunk_getpartial64", "pins_chunk_append64", "dmc_state_cam"}) {
//            if(!pins(func, false)) {
//                out.reportError("PINS api call not available: " + std::string(func));
//                pinsapiok = false;
//            } else {
//                out.reportNote("PINS api call available: " + std::string(func));
//            }
//        }
//        if(!pinsapiok) {
//            out.reportFailure("PINS api not adequate");
//            abort();
//        }

        // Determine all the transition groups from the code
        createTransitionGroups();

        generateBasicTypes();

        // Create the register mapping used to map registers to locations
        // in the state-vector
        createRegisterMapping();

        // Generate the LLVM types we need
        generateTypes();

        // Initialize the stack helper functions
        stack.init();

        // Generate the initial state
        generateInitialState();

        // Generate the contents of the next state function
        // for...
//        for(int i = 0; i < MAX_THREADS; ++i) {
//            generateNextState(i);
//        }
//        generateNextState();

        generateStepProcess();

        generateConstructorStart();
        generateMainStart();
        generateGetNextAllDMC();

        generateInterface();

        generateDebugInfo();


        //dmcModule->dump();

        out.reportAction("Verifying module...");
        out.indent();

//        for(auto& F: dmcModule->getFunctionList()) {
//            printf("F: %s\n", F.getName().str().c_str());
//            for(auto& BB: F) {
//                printf("  BB: %s\n", BB.getName().str().c_str());
//                for(auto& I: BB) {
//                    printf("    I: %s\n", I.getName().str().c_str());
//                    size_t ops = I.getNumOperands();
//                    for(size_t o = 0; o < ops; ++o) {
//                        auto O = I.getOperand(o);
//                        printf("      %u: %p\n", o, O);
//                        assert(O);
//                    }
//                }
//            }
//        }

        if(verifyModule(*dmcModule, &roout)) {
            roout << "\n";
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
     * @file LLDMCModelGenerator.h
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

        DIBuilder& dbuilder = *getDebugBuilder();

        DebugInfo dbgInfo;
        dbgInfo.TheCU = getCompileUnit();
        assert(dbgInfo.TheCU);
        assert(dbgInfo.TheCU->getFile());

//        dbgInfo.getnext = getDebugScopeForFunction(f_pins_getnextall2);
        dbgInfo.getnext = getDebugScopeForFunction(f_stepProcess);

        dbgInfo.moduleinit = getDebugScopeForFunction(f_dmc_initialstate);

        dbuilder.finalize();

        int l = 1;
        for(auto& F: *dmcModule) {
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
        NamedMDNode* culist = dmcModule->getNamedMetadata("llvm.dbg.cu");

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
        assert(ptr);
        assert(intToAdd);
        auto ptr_type = ptr->getType();
        ptr = builder.CreatePtrToInt(ptr, t_intptr);
        intToAdd = builder.CreateIntCast( intToAdd, t_intptr, false, "ptradd_inttoadd");
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
        auto s = folder.CreateGetElementPtr( type
                                                        , ConstantPointerNull::get(type->getPointerTo(0))
                                                        , {ConstantInt::get(t_int, 1)}
                                                        );
        s = folder.CreatePtrToInt(s, t_int);
        return s;
    }

    Constant* generateAlignedSizeOf(Value* data) {
        return generateAlignedSizeOf(data->getType(), c_bytewidth_statevar);
    }
    Constant* generateAlignedSizeOf(Type* type) {
        assert(c_bytewidth_statevar);
        return generateAlignedSizeOf(type, c_bytewidth_statevar);
    }
    Constant* generateAlignedSizeOf(Value* data, Constant* alignment) {
        return generateAlignedSizeOf(data->getType(), alignment);
    }
    Constant* generateAlignedSizeOf(Type* type, Constant* alignment) {
        auto s = folder.CreateGetElementPtr( type
                , ConstantPointerNull::get(type->getPointerTo(0))
                , {ConstantInt::get(t_int, 1)}
        );
        auto al1 = folder.CreateSub(alignment, ConstantInt::get(t_int, 1));

        s = folder.CreatePtrToInt(s, t_int);
        s = folder.CreateAdd(s, al1);
        s = folder.CreateAnd(s, folder.CreateNot(al1));
        return s;
    }

    size_t generateSizeOfNow(Value* data) {
        return generateSizeOfNow(data->getType());
    }
    size_t generateSizeOfNow(Type* type) {
        DataLayout* TD = new DataLayout(dmcModule);
        return TD->getTypeAllocSize(type);
    }

    GenerationContext generateContextAndSetupCode(Argument* self, Value* processorID, Argument* src, Argument* svout, Argument* user_context) {

        auto cpy = builder.CreateMemCpy( svout
                , svout->getPointerAlignment(dmcModule->getDataLayout())
                , src
                , src->getParamAlign()
                , t_statevector_size
        );
        setDebugLocation(cpy, __FILE__, __LINE__ - 6);

        GenerationContext context;

        context.thread_id = processorID;
        context.svout = svout;
        context.src = src;
        context.model = self;
        context.gen = this;
        context.alteredPC = false;
        context.userContext = user_context;

        return context;
    }

    void generateStepProcess() {

        // This generates:
        //
        // int pins_getnext2(self, procID, srcState, callback, context) {
        //   bool gotoNext = true;
        //   pc = srcState.processes[procID].pc
        //   while(pc && gotoNext) {
        //     switch(pc) {
        //       case 0:
        //          return 0;
        //       case TG0.pc:
        //          ...
        //       case TG1.pc:
        //          ...
        //       case TG2.pc:
        //          ...
        //     }
        //   }
        // }
        //
        //
        //

        f_stepProcess = Function::Create( t_stepProcess
                , GlobalValue::LinkageTypes::ExternalLinkage
                , "model_step"
                , dmcModule
        );

        // Get the arguments from the call
        auto args = f_stepProcess->arg_begin();
        Argument* self = &*args++;
        Argument* processorID = &*args++;
        Argument* stateID = &*args++;
        Argument* src = &*args++;
        Argument* svout = &*args++;
//        Argument* cb = &*args++;
        Argument* user_context = self;//&*args++;
//        Argument* transition_info = &*args++;
//        Argument* edgeLabelValue = &*args++;
        assert(svout);

        // Create entry block with a number of stack allocations
        BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_stepProcess);
        builder.SetInsertPoint(entry);
//        builder.CreateCall(pins("printf"), {generateGlobalString("---------------------- %u\n"), processorID});

//        ChunkMapper cm_action(gctx, type_action);
//        auto p_actionLabel = lts.getEdgeLabels()["action"].getValue(edgeLabelValue);
//        auto actionLabelChunkID = cm_action.generatePut(ConstantInt::get(t_int, 0), ConstantPointerNull::get(t_intp));
//        builder.CreateStore(actionLabelChunkID, p_actionLabel);

        auto cpy = builder.CreateMemCpy( svout
                , svout->getPointerAlignment(dmcModule->getDataLayout())
                , src
                , src->getParamAlign()
                , t_statevector_size
        );
        setDebugLocation(cpy, __FILE__, __LINE__ - 6);

//        Value* gotoNext = builder.CreateAlloca(t_bool);
//        builder.CreateStore(ConstantInt::get(t_bool, 1), gotoNext);
        auto dst_pc = lts["processes"][processorID]["pc"].getValue(svout);

        BasicBlock* process_active_check = BasicBlock::Create(ctx, "process_active_check" , f_stepProcess);
        BasicBlock* while_emitted = BasicBlock::Create(ctx, "while_emitted" , f_stepProcess);
        BasicBlock* while_notemitted = BasicBlock::Create(ctx, "while_notemitted" , f_stepProcess);
        BasicBlock* while_condition = BasicBlock::Create(ctx, "while_condition" , f_stepProcess);
        BasicBlock* while_body = BasicBlock::Create(ctx, "while_body" , f_stepProcess);
        BasicBlock* while_end = BasicBlock::Create(ctx, "while_end" , f_stepProcess);
        BasicBlock* nosuchpc = BasicBlock::Create(ctx, "nosuchpc" , f_stepProcess);
        BasicBlock* end_no_report = BasicBlock::Create(ctx, "end_no_report" , f_stepProcess);

//        builder.CreateCall(pins("printf"), {generateGlobalString("----- getnext\n")});
        builder.CreateBr(process_active_check);
        builder.SetInsertPoint(process_active_check);
        Value* pc1 = builder.CreateLoad(dst_pc);
        builder.CreateCondBr(builder.CreateICmpNE(pc1, ConstantInt::get(t_int, 0)), while_body, end_no_report);

        builder.SetInsertPoint(while_condition);
        auto emittedLastIteration = builder.CreatePHI(t_bool, 2);
        Value* pc2 = builder.CreateLoad(dst_pc);
        Value* pcIsNotZero = builder.CreateICmpNE(pc2, ConstantInt::get(t_int, 0));
        builder.CreateCondBr(pcIsNotZero, while_body, while_end);

        builder.SetInsertPoint(while_body);
        auto pc = builder.CreatePHI(t_int, 2);
        pc->addIncoming(pc1, process_active_check);
        pc->addIncoming(pc2, while_condition);
        auto emitted = builder.CreatePHI(t_bool, 2);
        emitted->addIncoming(ConstantInt::get(t_bool, 0), process_active_check);
        emitted->addIncoming(emittedLastIteration, while_condition);
//        builder.CreateCall(pins("printf"), {generateGlobalString("STEP processorID %u, pc %u\n"), processorID, pc});
        SwitchInst* swtch = builder.CreateSwitch(pc, nosuchpc, transitionGroups.size());

        emittedLastIteration->addIncoming(ConstantInt::get(t_bool, 1), while_emitted);
        emittedLastIteration->addIncoming(emitted, while_notemitted);

        GenerationContext context;
        GenerationContext* gctx = &context;

        context.thread_id = processorID;
        context.svout = svout;
        context.src = src;
        context.model = self;
        context.gen = this;
        context.alteredPC = false;
        context.userContext = user_context;
        context.noReportBB = end_no_report;

        builder.SetInsertPoint(while_emitted);
        builder.CreateBr(while_condition);
        builder.SetInsertPoint(while_notemitted);
        builder.CreateBr(while_condition);

        // Per transition group, add an entry to the switch, the bodies of
        // the cases will be populated later
        for(size_t i = 0; i < transitionGroups.size(); ++i) {
            auto& t = transitionGroups[i];
            if(t->getType() == TransitionGroup::Type::Instructions) {
                auto ti = static_cast<TransitionGroupInstructions*>(t);
                if(ti->thread_id == 0) { // temp hack to make sure we do this only once, instead of per processor
                    BasicBlock* pcBBemitcheck = BasicBlock::Create(ctx, "pc_emitcheck", f_stepProcess);
                    BasicBlock* pcBB = BasicBlock::Create(ctx, "pc", f_stepProcess);

                    size_t currentLocationIndex = programLocations[ti->instructions.front()];

//                    raw_os_ostream ros(std::cout);
//                    ros << "[" << currentLocationIndex << "] " << *ti->instructions.front() << "\n";
//                    ros.flush();

                    swtch->addCase(ConstantInt::get(t_int, currentLocationIndex), pcBBemitcheck);
                    builder.SetInsertPoint(pcBB);
//                    builder.CreateCall(pins("printf"), {generateGlobalString("Instruction at PC: %u\n"), builder.CreateLoad(dst_pc)});
                    bool emitter = generateNextStateForGroup(ti, gctx, end_no_report, f_stepProcess);
                    if(emitter) {
                        builder.CreateBr(while_emitted);
                        builder.SetInsertPoint(pcBBemitcheck);
                        builder.CreateCondBr(emitted, while_end, pcBB);
                    } else {
                        builder.CreateBr(while_notemitted);
                        builder.SetInsertPoint(pcBBemitcheck);
                        builder.CreateBr(pcBB);
                    }
                }
            }
        }

        // Basic block for when the specified group ID is invalid
        builder.SetInsertPoint(nosuchpc);
        builder.CreateCall(pins("printf"), {generateGlobalString("No such program counter: %u\n"), pc});
        builder.CreateRet(ConstantInt::get(t_int64, 0));

        builder.SetInsertPoint(end_no_report);
        builder.CreateRet(ConstantInt::get(t_int64, 0));

        builder.SetInsertPoint(while_end);

//        gctx->edgeLabels["action"] = actionLabelChunkID;
//        builder.CreateCall(pins("printf"), {generateGlobalString("Instruction: " + gctx->strAction + "\n")});

//                    Value* ch = cm_action.generateGet(actionLabelChunkID);
//                    Value* ch_data = gctx->gen->generateChunkGetData(ch);
//                    Value* ch_len = gctx->gen->generateChunkGetLen(ch);

        // DEBUG: print labels
//        for(auto& nameValue: gctx->edgeLabels) {
//            auto value = lts.getEdgeLabels()[nameValue.first].getValue(gctx->edgeLabelValue);
//            builder.CreateStore(nameValue.second, value);
//        }

//        builder.CreateCall(pins("printf"), {generateGlobalString("Emitting PC: %u\n"), builder.CreateLoad(dst_pc)});
        StateManager sm_root(user_context, this, lts.getSV().getType());
        sm_root.uploadBytes(stateID, svout, t_statevector_size);
        builder.CreateRet(ConstantInt::get(t_int64, 1));

    }

    bool generateNextStateForGroup(TransitionGroupInstructions* ti, GenerationContext* gctx, BasicBlock* noReportBB, Function* func) {

        auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);

        auto bb_transition = BasicBlock::Create(ctx, "bb_transition", func);
//        if(ti->instructions.size() > 0) {
//            Value* condition = ConstantInt::get(t_bool, 1);
//            for(auto I: ti->instructions) {
//                auto instruction_condition = generateConditionForInstruction(gctx, I);
//                if(instruction_condition) {
//                    condition = builder.CreateAnd(condition, instruction_condition);
//                }
//            }
//            builder.CreateCondBr(condition, bb_transition, noReportBB);
//        } else {
            builder.CreateBr(bb_transition);
//        }

        builder.SetInsertPoint(bb_transition);

        auto locID = programLocations[ti->instructions.back()->getNextNode()];

        // Update the destination PC
        builder.CreateStore(ConstantInt::get(t_int, locID), dst_pc);

        // Debug
        std::string strAction;
        raw_string_ostream strout(strAction);

        bool emitter = false;

        // Generate the next state for the instructions
        for(auto I: ti->instructions) {
            if(!isRuntimeCollapsableInstruction(I)) {
                strout << "; ";
                if(valueRegisterIndex.count(I)) {
                    strout << "r[" << valueRegisterIndex[I] << "] ";
                }
                strout << " {";
                strout << I->getFunction()->getName();
                strout << "} ";
                strout << *I;
                emitter = true;
            }
            generateNextStateForInstruction(gctx, I);

//            std::string sInstruction;
//            raw_string_ostream stroutInstruction(sInstruction);
//            stroutInstruction << *I;
//
//            std::string s;
//            raw_string_ostream strout2(s);
//            strout2 << "[";
//            strout2 << I->getFunction()->getName();
//            strout2 << ":" << programLocations[I];
//            strout2 << "] %s";
//            if(valueRegisterIndex.count(I)) {
//                strout2 << " = \033[1m%zx\033[0m (";
//                strout2 << valueRegisterIndex[I];
//                strout2 << ")\n";
//                builder.CreateCall(pins("printf"), {generateGlobalString(strout2.str()), generateGlobalString(stroutInstruction.str()), vMap(gctx, I)});
//            } else {
//                strout2 << "\n";
//                builder.CreateCall(pins("printf"), {generateGlobalString(strout2.str()), generateGlobalString(stroutInstruction.str())});
//            }
        }

        strout << "   ";
        strout.flush();

        // Generate the next state for all other actions
        for(auto& action: ti->actions) {
            generateNextStateForInstruction(gctx, action);
        }

        // Generate the action label
        if(emitter) {
            // TODO: dmc api needs this
        }

        return emitter;

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
    }

    void generateConstructorStart() {
        f_constructorStart = Function::Create( t_globalStep
                , GlobalValue::LinkageTypes::ExternalLinkage
                , "init_constructor_start"
                , dmcModule
        );

        // Get the arguments from the call
        auto args = f_constructorStart->arg_begin();
        Argument* self = &*args++;
        Argument* src = &*args++;
        Argument* svout = &*args++;
//        Argument* cb = &*args++;
        Argument* user_context = self;
//        Argument* transition_info = &*args++;
//        Argument* edgeLabelValue = &*args++;

        BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_constructorStart);
        builder.SetInsertPoint(entry);

        GenerationContext gctx = generateContextAndSetupCode(self, ConstantInt::get(t_int, 0), src, svout, user_context);

        auto func = module->getFunction("_GLOBAL__sub_I_test.cpp"); // This name needs to change
        if(func) {
            stack.pushStackFrame(&gctx, *func, {}, nullptr);
        }

        builder.CreateRet(ConstantInt::get(t_int64, 0));

    }

    void generateMainStart() {
        f_mainStart = Function::Create( t_globalStep
                , GlobalValue::LinkageTypes::ExternalLinkage
                , "init_main_start"
                , dmcModule
        );

        // Get the arguments from the call
        auto args = f_mainStart->arg_begin();
        Argument* self = &*args++;
        Argument* src = &*args++;
        Argument* svout = &*args++;
//        Argument* cb = &*args++;
        Argument* user_context = self;//&*args++;
//        Argument* transition_info = &*args++;
//        Argument* edgeLabelValue = &*args++;

        BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_mainStart);
        builder.SetInsertPoint(entry);

        GenerationContext gctx = generateContextAndSetupCode(self, ConstantInt::get(t_int, 0), src, svout, user_context);

        auto func = module->getFunction("main"); // This name needs to change
        if(func) {
            stack.pushStackFrame(&gctx, *func, {}, nullptr);
        }

        builder.CreateRet(ConstantInt::get(t_int64, 0));

    }

    void generateGetNextAllDMC() {
        f_dmc_nextstates = Function::Create( t_dmc_nextstates
                , GlobalValue::LinkageTypes::ExternalLinkage
                , "dmc_nextstates"
                , dmcModule
        );

        // Get the arguments from the call
        auto args = f_dmc_nextstates->arg_begin();
        Argument* self = &*args++;
        Argument* stateID = &*args++;
//        Argument* cb = &*args++;
        Argument* user_context = self;//&*args++;

        // Basic blocks needed for the for loop
        BasicBlock* entry = BasicBlock::Create(ctx, "entry" , f_dmc_nextstates);
        BasicBlock* constructors_start  = BasicBlock::Create(ctx, "constructors_start" , f_dmc_nextstates);
        BasicBlock* main_start  = BasicBlock::Create(ctx, "main_start" , f_dmc_nextstates);
        BasicBlock* process_loop  = BasicBlock::Create(ctx, "process_loop" , f_dmc_nextstates);
        BasicBlock* forcond = BasicBlock::Create(ctx, "for_cond" , f_dmc_nextstates);
        BasicBlock* forbody = BasicBlock::Create(ctx, "for_body" , f_dmc_nextstates);
        BasicBlock* forincr = BasicBlock::Create(ctx, "for_incr" , f_dmc_nextstates);
        BasicBlock* incrstatusifneeded = BasicBlock::Create(ctx, "incrstatusifneeded" , f_dmc_nextstates);
        BasicBlock* incrstatus = BasicBlock::Create(ctx, "incrstatus" , f_dmc_nextstates);
        BasicBlock* wrongstatus = BasicBlock::Create(ctx, "wrongstatus" , f_dmc_nextstates);
        BasicBlock* endstate = BasicBlock::Create(ctx, "endstate" , f_dmc_nextstates);
        BasicBlock* end = BasicBlock::Create(ctx, "end" , f_dmc_nextstates);

        // Setup the switch
        builder.SetInsertPoint(entry);

        StateManager sm_root(self, this, lts.getSV().getType());
        Value* stateLength = sm_root.getLength(stateID);
        Value* src = sm_root.download(stateID);

        Value* svout = builder.CreateAlloca(IntegerType::get(ctx, 32), stateLength);
        svout = builder.CreatePointerCast(svout, t_statevector->getPointerTo());

        Value* p_status = lts["status"].getValue(src);
        Value* status = builder.CreateLoad(t_int, p_status, "status");
        SwitchInst* swtch = builder.CreateSwitch(status, wrongstatus, 3);
        swtch->addCase(ConstantInt::get(t_int, GlobalStatus::START), constructors_start);
        swtch->addCase(ConstantInt::get(t_int, GlobalStatus::RUNNING_CONSTRUCTORS), process_loop);
        swtch->addCase(ConstantInt::get(t_int, GlobalStatus::STARTING_MAIN), main_start);
        swtch->addCase(ConstantInt::get(t_int, GlobalStatus::RUNNING_MAIN), process_loop);
//        swtch->addCase(ConstantInt::get(t_int, GlobalStatus::ENDED), endstate);

//        for(int i = 0; i < MAX_THREADS; ++i) {
//            auto src_pc = lts["processes"][i]["pc"].getValue(gctx_copy.src);
//            auto pc = builder.CreateLoad(t_int, src_pc, "pc");
//            auto cond2 = builder.CreateICmpEQ(pc, ConstantInt::get(t_int, 0));
//            cond = builder.CreateAnd(cond, cond2);
//        }


        // Constructor start
        builder.SetInsertPoint(constructors_start);
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("constructors_start: %u\n")
//                                    , status
//                            }
//        );
        builder.CreateCall(f_constructorStart, {user_context, src, svout});
        builder.CreateStore(ConstantInt::get(t_int, 1), lts["status"].getValue(svout));
        sm_root.uploadBytes(stateID, svout, t_statevector_size);
        builder.CreateRet(ConstantInt::get(t_int64, 1));

        // Main start
        builder.SetInsertPoint(main_start);
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("main_start: %u\n")
//                                    , status
//                            }
//        );
        builder.CreateCall(f_mainStart, {user_context, src, svout});
        builder.CreateStore(ConstantInt::get(t_int, 3), lts["status"].getValue(svout));
        sm_root.uploadBytes(stateID, svout, t_statevector_size);
        builder.CreateRet(ConstantInt::get(t_int64, 1));

        // Process loop
        builder.SetInsertPoint(process_loop);
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("process_loop: %u\n")
//                                    , status
//                            }
//        );
        builder.CreateBr(forcond);

        // The condition checks that the current processorID is
        // smaller than the total number of processors
        builder.SetInsertPoint(forcond);
        PHINode* tg = builder.CreatePHI(t_int, 2, "ProcessorID");
        PHINode* emitted = builder.CreatePHI(t_int64, 2, "emitted");
        Value* cond = builder.CreateICmpULT(tg, ConstantInt::get(t_int, MAX_THREADS));
        builder.CreateCondBr(cond, forbody, incrstatusifneeded);

        // The body of the for loop: calls pins_getnext for very processorID
        builder.SetInsertPoint(forbody);
        Value* emittedByProcessStep = builder.CreateCall(f_stepProcess, {user_context, tg, stateID, src, svout});
        builder.CreateBr(forincr);

        // Increment tg
        builder.SetInsertPoint(forincr);
        Value* nextP = builder.CreateAdd(tg, ConstantInt::get(t_int, 1));

        // Tell the Phi node of the incoming edges
        tg->addIncoming(ConstantInt::get(t_int, 0), process_loop);
        tg->addIncoming(nextP, forincr);
        emitted->addIncoming(ConstantInt::get(t_int64, 0), process_loop);
        emitted->addIncoming(builder.CreateAdd(emittedByProcessStep, emitted), forincr);
        builder.CreateBr(forcond);

        builder.SetInsertPoint(incrstatusifneeded);
        builder.CreateCondBr(builder.CreateICmpEQ(ConstantInt::get(t_int64, 0), emitted), incrstatus, end);

        builder.SetInsertPoint(incrstatus);
//        builder.CreateStore(builder.CreateAdd(ConstantInt::get(t_int, 1), status), p_status);
//        builder.CreateCall( pins("printf")
//                          , { generateGlobalString("incremented status from: %u\n")
//                            , status
//                            }
//        );
        sm_root.deltaBytes(stateID, ConstantInt::get(t_int, 0), builder.CreateAdd(ConstantInt::get(t_int, 1), status));

        builder.CreateRet(ConstantInt::get(t_int64, 1));

        builder.SetInsertPoint(endstate);
//        builder.CreateCall( pins("printf")
//                          , { generateGlobalString("end state: %u\n")
//                            , status
//                            }
//        );
        builder.CreateRet(ConstantInt::get(t_int64, 0));

        builder.SetInsertPoint(wrongstatus);
//        builder.CreateCall( pins("printf")
//                          , { generateGlobalString("wrong status: %u\n")
//                            , status
//                            }
//        );
        builder.CreateRet(ConstantInt::get(t_int64, 0));

        // Finally return 0
        builder.SetInsertPoint(end);
        builder.CreateRet(emitted);
    }

    void createRegisterMapping() {

        unsigned int nextID = 0;
        auto newmap = [&nextID, this](Function* F, Value* V) {
            registerLayout[F].registerTypes.push_back(V->getType());
            valueRegisterIndex[V] = nextID++;
        };

        // Globals
        // Assign every global an index, starting at 0
//        nextID = 0;
//        for(auto& v: module->getGlobalList()) {
//            valueRegisterIndex[&v] = nextID++;
//        }

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
            auto regStruct = StructType::create( ctx
                                               , registerLayout[&F].registerTypes
                                               , "t_" + F.getName().str() + "_registers"
                                               , false
                                               );
            auto& DL = dmcModule->getDataLayout();
            auto layout = DL.getStructLayout(regStruct);
            registerLayout[&F].registerLayout = regStruct;

//            roout << "Creating register layout for function " << F.getName().str() << ": \n";
//            for(size_t idx = 0; idx < regStruct->getNumElements(); ++idx) {
//                auto a = layout->getElementOffset(idx);
//                roout << " - " << *regStruct->getElementType(idx) << " @ " << a << "\n";
//            }
//            roout << *registerLayout[&F].registerLayout << "\n";
//            roout.flush();
            assert(nextID == registerLayout[&F].registerTypes.size());
        }
    }

    /**
     * @brief Generates the types to be used and generates the LTS type
     */
    void generateBasicTypes() {
        // Basic types
        t_bool = IntegerType::get(ctx, 1);
        t_int = IntegerType::get(ctx, BITWIDTH_STATEVAR);
        t_intp = t_int->getPointerTo(0);
        t_int8 = IntegerType::get(ctx, 8);
        t_int64 = IntegerType::get(ctx, 64);
        t_int64p = PointerType::get(t_int64, 0);
        t_intptr = IntegerType::get(ctx, 64); // FIXME
        t_void = PointerType::getVoidTy(ctx);
        t_char = IntegerType::get(ctx, 8);
        t_charp = PointerType::get(t_char, 0);
        t_voidp = t_charp; //PointerType::get(PointerType::getVoidTy(ctx), 0);
        t_voidpp = PointerType::get(t_voidp, 0);

        // Chunk types
        t_chunkid = t_int64;

        t_pthread_t = IntegerType::get(ctx, 64);

        ptr_offset_threadid = ConstantInt::get(t_intptr, 40);
        ptr_mask_threadid = ConstantInt::get(t_intptr, (1ULL << 40) - 1);
        ptr_offset_location = ConstantInt::get(t_intptr, 24);
        ptr_mask_location = ConstantInt::get(t_intptr, (1ULL << 24) - 1);

        c_globalMemStart = ConstantInt::get(t_intptr, 0); // fixme: should not be constant like this
        c_bytewidth_statevar = ConstantInt::get(t_int, BITWIDTH_STATEVAR/8);

        out.reportAction("Types");
        {
            std::string s;
            raw_string_ostream ros(s);
            ros << "t_chunkid: " << *t_chunkid;
            out.reportAction2(ros.str());
        }

    }

    void generateTypes() {
        assert(t_statevector == nullptr);

        // Determine the register size
        out.reportAction("Determining register size in state vector");
        out.indent();
        auto& DL = dmcModule->getDataLayout();
        size_t registersInBits = 0;

//        Value* maxSize = ConstantInt::get(t_int, 0);

        for(auto& kv: registerLayout) {
            if(!kv.second.registerLayout->isSized()) {
                roout << "Type is not sized: " << *kv.second.registerLayout << "\n";
                roout.flush();
                assert(0);
            }
            auto bitsNeeded = DL.getTypeSizeInBits(kv.second.registerLayout);
//            std::stringstream ss;
//            ss << kv.first->getName().str() << " needs " << (bitsNeeded / 8) << " bytes";
//            out.reportNote(ss.str());
            if(registersInBits < bitsNeeded) {
                registersInBits = bitsNeeded;
            }

//            auto newSize = builder.CreateGEP( kv.second.registerLayout,
//                                              ConstantPointerNull::get(kv.second.registerLayout->getPointerTo()),
//                                              {ConstantInt::get(t_int, 1)});
//            auto cond = builder.CreateICmpULT(maxSize, newSize);
//            maxSize = builder.CreateSelect(cond, newSize, maxSize);

        }
        out.outdent();
//        t_registers_max_size = maxSize;
        t_registers_max = ArrayType::get(t_int, registersInBits / 32); // needs to be based on max #registers of all functions
        t_registers_max_size = builder.CreateGEP(t_registers_max,
                                                 ConstantPointerNull::get(t_registers_max->getPointerTo()),
                                                 {ConstantInt::get(t_int, 1)}
        );
        std::stringstream ss;
        ss << "Determined required number of registers: " << (registersInBits / 32) << " slots";
        out.reportAction(ss.str());

        t_registers_max_size = builder.CreatePtrToInt(t_registers_max_size, t_int);

        // Creates types to use in the LTS type
        type_status = typeManager.newType("status", t_int
        );
        type_pc = typeManager.newType("pc", t_int
        );
        type_tid = typeManager.newType("tid", t_int
        );
        type_threadresults = typeManager.newType("threadresults", t_chunkid
        );
        type_stack = typeManager.newType("stack", t_chunkid
        );
        type_register_frame = typeManager.newType("rframe", t_chunkid
        );
        type_registers = typeManager.newType("register", t_registers_max
        );
        type_memory = typeManager.newType("memory", t_chunkid
        );
        type_size = typeManager.newType("msize", t_int
        );
//        type_heap = typeManager.newType("heap", t_chunkid
//        );
        type_action = typeManager.newType("action", t_chunkid
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
        lts << type_size;
//        lts << type_heap;
        lts << type_action;

        {
            auto sType = typeManager.newType("status", t_int64);
            auto vType = typeManager.newType("value", t_voidp);
            root_threadresults_element = new SVRoot(&typeManager, "tres_element");
            *root_threadresults_element << new SVTree("status", sType)
                                        << new SVTree("value", vType)
            ;
            type_threadresults_element = root_threadresults_element->getType();
        }

        root_threadresults = SVTree::newArray("results", "results", type_threadresults_element);


        // Create the state-vector tree of variable nodes
        auto sv = new SVRoot(&typeManager, "type_sv");

        // Processes
//        auto sv_processes = new SVTree("processes", "processes");
//        for(int i = 0; i < MAX_THREADS; ++i) {
            auto sv_proc = new SVTree("proc", "process");
            *sv_proc
                    << new SVTree("status", type_status)
                    << new SVTree("tid", type_tid)
                    << new SVTree("pc", type_pc)
                    << new SVTree("msize", type_size)
                    << new SVTree("stk", type_stack)
                    << new SVTree("r", type_registers)
                    << new SVTree("m", type_memory)
                    ;
//            *sv_processes << sv_proc;
//        }
        auto sv_processes = SVTree::newArray("processes", "processes", sv_proc, MAX_THREADS);

        // Types
        auto structTypes = module->getIdentifiedStructTypes();
//        std::cout << "ID'd structs:" << std::endl;
//        for(auto& st: structTypes) {
//            std::cout << "  - " << st->getName().str() << " : " << st->getStructName().str() << std::endl;
//        }

//        llvm::TypeFinder StructTypes;
//        StructTypes.run(*module, true);

//        std::cout << "ID'd structs:" << std::endl;
//        for(auto* STy : StructTypes) {
//            std::cout << "  - " << STy->getName().str() << " : " << STy->getStructName().str() << std::endl;
//        }

//        // Globals
        std::vector<Type*> globals;
        size_t nextID = 0;
        for(auto& t: module->getGlobalList()) {
            if(t.hasInitializer()) {
//                if(t.getInitializer()->isZeroValue()) {
//                    continue;
//                }
                if(t.getName().equals("llvm.global_ctors")) {
                    continue;
                }
                mappedConstantGlobalVariables[&t] = new GlobalVariable(*dmcModule, t.getValueType(), t.isConstant(),
                                                                       t.getLinkage(), nullptr, t.getName());
            }
            valueRegisterIndex[&t] = nextID++;
            globals.push_back(t.getType()->getPointerElementType());
        }
        t_globals = StructType::get(ctx, globals, true);

        for(auto& t: module->getGlobalList()) {
            if(t.hasInitializer()) {
//                if(t.getInitializer()->isZeroValue()) {
//                    continue;
//                }
                if(t.getName().equals("llvm.global_ctors")) {
                    continue;
                }
                Value* init = t.getInitializer();
                init = vMap(nullptr, init);
                assert(isa<Constant>(init));
                mappedConstantGlobalVariables[&t]->setInitializer(dyn_cast<Constant>(init));
            }
        }



//        // Globals
//        auto sv_globals = new SVTree("globals", "globals");
//        for(auto& t: module->getGlobalList()) {
//
//            // Determine the type of the global
//            PointerType* pt = dyn_cast<PointerType>(t.getType());
//            assert(pt && "global is not pointer... constant?");
//            auto gt = pt->getElementType();
//
//            // Add the global
//            auto type_global = typeManager.newType( "global_" + t.getName().str()
//                                                  , LTStypeSInt32
//                                                  , gt
//                                                  );
//            lts << type_global;
//            *sv_globals << new SVTree(t.getName(), type_global);
//        }

        // Heap
//        auto sv_heap = new SVTree("memory", type_memory);

        // State-vector specification
        *sv
            << new SVTree("status", type_status)
            << new SVTree("threadsStarted", type_tid)
            << new SVTree("tres", type_threadresults)
//            << sv_heap
            << sv_processes
            ;



        // Only add globals if there are any
//        if(sv_globals->count() > 0) {
//            *sv << sv_globals;
//        }

        // Edge label specification
        auto edges = new SVEdgeLabelRoot(&typeManager, "type_edgelabels");
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

        roout << "t_statevector: " << *t_statevector << "\n";
        roout << "t_edgeLabels:  " << *t_edgeLabels << "\n";

        // int (model_t self, int const* src, TransitionCB cb, void* user_context)
        t_dmc_initialstate = FunctionType::get(t_int64,{ t_voidp}, false);
        t_dmc_nextstates = FunctionType::get(t_int64,{ t_voidp, t_int64}, false);

        t_globalStep = FunctionType::get( t_int64
                                        , { t_voidp
                                          , PointerType::get(t_statevector, 0)
                                          , PointerType::get(t_statevector, 0)
                                          }
                                        , false
        );

        t_stepProcess = FunctionType::get( t_int64
                                         , { t_voidp
                                           , t_int
                                           , t_int64
                                           , PointerType::get(t_statevector, 0)
                                           , PointerType::get(t_statevector, 0)
                                           }
                                         , false
        );

        // Get the size of the SV
        t_statevector_size = generateAlignedSizeOf(t_statevector);
        assert(t_statevector_size);
    }

    void generateInitialState() {
        s_statevector = new GlobalVariable( *dmcModule
                                          , t_statevector
                                          , false
                                          , GlobalValue::ExternalLinkage
                                          , ConstantAggregateZero::get(t_statevector)
                                          , "s_initstate"
                                          );
    }

    /**
     * @brief Creates transition groups that all pinsified modules need, like
     * the call to main() to start the program
     */
    void createDefaultTransitionGroups() {

        // Generate a TG to start all constructors
        addTransitionGroup(new TransitionGroupLambda([=](GenerationContext* gctx, TransitionGroupLambda* t, BasicBlock* noReportBB) {

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

            GenerationContext gctx_copy = *gctx;
            gctx_copy.thread_id = ConstantInt::get(t_int, 0);

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

            BasicBlock* constructors_start  = BasicBlock::Create(ctx, "constructors_start" , F);
            builder.CreateCondBr(cond, constructors_start, noReportBB);

            builder.SetInsertPoint(constructors_start);
//            builder.CreateCall(pins("printf"), {generateGlobalString("main_start\n")});
            auto cpy = builder.CreateMemCpy(gctx_copy.svout, gctx_copy.svout->getPointerAlignment(dmcModule->getDataLayout()), src, src->getParamAlign(), t_statevector_size);
            setDebugLocation(cpy, __FILE__, __LINE__ - 1);
//            for(int i = 0; i < MAX_THREADS; ++i) {
//                gctx_copy.thread_id = i;
//            GlobalVariable* cList = module->getGlobalVariable("llvm.global_ctors", true);
//            Constant* cListV = cList->getInitializer();
//            auto cListV2 = dyn_cast<ConstantArray>(cListV);
//            assert(cListV2);
//            assert(cListV2->getNumElements() < 2);
//            if(cListV2->getNumElements() > 0) {
//                auto e = cListV2->getAggregateElement(0U);
//                assert(e);
//                auto user = dyn_cast<User>(e);
                auto func = module->getFunction("_GLOBAL__sub_I_test.cpp");
                if(func) {
                    stack.pushStackFrame(&gctx_copy, *func, {}, nullptr);
                }
//            }

//            }
            //builder.CreateStore(ConstantInt::get(t_int, 1), svout_pc[0]);
            builder.CreateStore(ConstantInt::get(t_int, 1), lts["status"].getValue(gctx_copy.svout));

        }
        ));

        // Generate a TG to start main()
        addTransitionGroup(new TransitionGroupLambda([=](GenerationContext* gctx, TransitionGroupLambda* t, BasicBlock* noReportBB) {

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

            GenerationContext gctx_copy = *gctx;
            gctx_copy.thread_id = ConstantInt::get(t_int, 0);

            // Generate the check that all threads are inactive and main()
            // has not been called before
            Value* cond = lts["status"].getValue(gctx_copy.src);
            cond = builder.CreateLoad(t_int, cond, "status");
            cond = builder.CreateICmpEQ(cond, ConstantInt::get(t_int, 1));
            for(int i = 0; i < MAX_THREADS; ++i) {
                auto src_pc = lts["processes"][i]["pc"].getValue(gctx_copy.src);
                auto pc = builder.CreateLoad(t_int, src_pc, "pc");
                auto cond2 = builder.CreateICmpEQ(pc, ConstantInt::get(t_int, 0));
                cond = builder.CreateAnd(cond, cond2);
            }

            BasicBlock* main_start  = BasicBlock::Create(ctx, "main_start" , F);
            builder.CreateCondBr(cond, main_start, noReportBB);

            builder.SetInsertPoint(main_start);
//            builder.CreateCall(pins("printf"), {generateGlobalString("main_start\n")});
            auto cpy = builder.CreateMemCpy(gctx_copy.svout, gctx_copy.svout->getPointerAlignment(dmcModule->getDataLayout()), src, src->getParamAlign(), t_statevector_size);
            setDebugLocation(cpy, __FILE__, __LINE__ - 1);
//            for(int i = 0; i < MAX_THREADS; ++i) {
//                gctx_copy.thread_id = i;
                stack.pushStackFrame(&gctx_copy, *module->getFunction("main"), {}, nullptr);
//            }
            //builder.CreateStore(ConstantInt::get(t_int, 1), svout_pc[0]);
            builder.CreateStore(ConstantInt::get(t_int, 2), lts["status"].getValue(gctx_copy.svout));

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
//        out << "Transition groups..." << std::endl;
//        out.indent();
        for(auto& F: *module) {
            if(F.isDeclaration()) continue;
//            out << "Function [" << F.getName().str() << "]" <<  std::endl;
            out.indent();
            createTransitionGroupsForFunction(F);
            out.outdent();
        }
//        out.outdent();
//        out << "groups: " << transitionGroups.size() <<  std::endl;
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
        return isFrontCollapsableInstruction(I) && isBackCollapsableInstruction(I);
    }

    bool isFrontCollapsableInstruction(Instruction* I) {
        switch(I->getOpcode()) {
            case Instruction::Alloca:
                return true;
            case Instruction::Load: {
                LoadInst* L = dyn_cast<LoadInst>(I);
                return _assumeNonAtomicCollapsable && !L->isAtomic();
            } case Instruction::Store: {
                StoreInst* S = dyn_cast<StoreInst>(I);
                return _assumeNonAtomicCollapsable && !S->isAtomic();
            } case Instruction::Ret:
                return true; // I think this is fine
            case Instruction::Br:
            case Instruction::Switch:
            case Instruction::IndirectBr:
                // Branches are not collapsable because PHI nodes are handled when they happen
                return true;
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
            case Instruction::Call: {
                CallInst* call = dyn_cast<CallInst>(I);
                if(call->getIntrinsicID() != Intrinsic::not_intrinsic) {
                    return true;
                }
                if(call->isInlineAsm()) return false;
                Function* F = call->getCalledFunction();
                if(!F) return false;
                if(F->isDeclaration()) {
                    if(F->getName().equals("malloc")) {
                        return true;
                    }
                    if(F->getName().equals("pthread_join")) {
                        return true;
                    }
                    return false;
                }
                return false; // sadly the rest of the llvm front-end does not read the updated PC
                return true;
            } case Instruction::VAArg:
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

    bool isBackCollapsableInstruction(Instruction* I) {
        switch(I->getOpcode()) {
            case Instruction::Alloca:
                return true;
            case Instruction::Load: {
                LoadInst* L = dyn_cast<LoadInst>(I);
                return _assumeNonAtomicCollapsable && !L->isAtomic();
            } case Instruction::Store: {
                StoreInst* S = dyn_cast<StoreInst>(I);
                return _assumeNonAtomicCollapsable && !S->isAtomic();
            } case Instruction::Ret:
                return true; // I think this is fine
            case Instruction::Br:
            case Instruction::Switch:
            case Instruction::IndirectBr:
                // Branches are not collapsable because PHI nodes are handled when they happen
                return true;
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
            case Instruction::Call: {
                CallInst* call = dyn_cast<CallInst>(I);
                if(call->getIntrinsicID() != Intrinsic::not_intrinsic) {
                    return true;
                }
                if(call->isInlineAsm()) return false;
                Function* F = call->getCalledFunction();
                if(!F) return false;
                if(F->isDeclaration()) {
                    if(F->getName().equals("malloc")) {
                        return true;
                    }
                    return false;
                }
                return false; // sadly the rest of the llvm front-end does not read the updated PC
                return true;
            } case Instruction::VAArg:
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

    bool isRuntimeCollapsableInstruction(Instruction* I) {
        switch(I->getOpcode()) {
            case Instruction::Alloca:
                return true;
            case Instruction::Load: {
                LoadInst* L = dyn_cast<LoadInst>(I);
                return _assumeNonAtomicCollapsable && !L->isAtomic();
            } case Instruction::Store: {
                StoreInst* S = dyn_cast<StoreInst>(I);
                return _assumeNonAtomicCollapsable && !S->isAtomic();
            } case Instruction::Ret:
                return true; // I think this is fine
            case Instruction::Br:
            case Instruction::Switch:
            case Instruction::IndirectBr:
                // Branches are not collapsable because PHI nodes are handled when they happen
                return true;
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
            case Instruction::Call: {
                CallInst* call = dyn_cast<CallInst>(I);
                if(call->getIntrinsicID() != Intrinsic::not_intrinsic) {
                    return true;
                }
                if(call->isInlineAsm()) return false;
                Function* F = call->getCalledFunction();
                if(!F) return false;
                if(F->isDeclaration()) {
                    if(F->getName().equals("malloc")) {
                        return true;
                    }
                    return false;
                }
                return true;
            } case Instruction::VAArg:
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
    bool instructionsCanCollapse(Instruction* before, Instruction* after) {
        if(before->getOpcode() == Instruction::Call && after->getOpcode() == Instruction::Call) {
            auto beforeCall = dyn_cast<CallInst>(before);
            auto afterCall = dyn_cast<CallInst>(after);
            Function* beforeCallFunction = beforeCall->getCalledFunction();
            Function* afterCallFunction = afterCall->getCalledFunction();
            if(!beforeCallFunction) return false;
            if(!afterCallFunction) return false;
            if(beforeCallFunction->getName().equals("pthread_create") && afterCallFunction->getName().equals("pthread_create")) {
                return true;
            }
            if(beforeCallFunction->getName().equals("pthread_join") && afterCallFunction->getName().equals("pthread_join")) {
                return true;
            }
            if(beforeCallFunction->getName().equals("__LLMCOS_Thread_New") && afterCallFunction->getName().equals("__LLMCOS_Thread_New")) {
                return true;
            }
            if(beforeCallFunction->getName().equals("__LLMCOS_Thread_Join") && afterCallFunction->getName().equals("__LLMCOS_Thread_Join")) {
                return true;
            }
        }
        return false;
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

        while(It != IE && isFrontCollapsableInstruction(&*It)) {

            auto I = &*It;
            rdesc << I;

            // Assign a PC to the instruction if it does not have one yet
            if(programLocations[I] == 0) {
                programLocations[I] = nextProgramLocation++;
            }

            instructions.push_back(I);

            It++;
        }

        bool emitter = false;

        if(It != IE) {

            Instruction* mainInstruction = &*It;

            emitter = true;
            for(;;) {

                auto I = &*It;
                rdesc << I;

                // Assign a PC to the instruction if it does not have one yet
                if(programLocations[I] == 0) {
                    programLocations[I] = nextProgramLocation++;
                }

                instructions.push_back(I);

                It++;

                if(It == IE || (!isBackCollapsableInstruction(&*It) && !instructionsCanCollapse(mainInstruction, &*It))) {
                    break;
                }

                // Ugly hack: calls cannot allow instructions afterwards
                if(I->getOpcode() == Instruction::Call) {
                    break;
                }

            }
            if(It != IE && programLocations[&*It] == 0) {
                programLocations[&*It] = nextProgramLocation++;
            }
        }

        // Create the TG
        for(int i = 0; i < MAX_THREADS; ++i) {
            auto tg = new TransitionGroupInstructions(i, instructions, {}, {}, emitter);
            tg->setDesc(rdesc.str() + (emitter ? " [emitter]" : ""));
            addTransitionGroup(tg);
        }

        // Print
//        out << "  " << programLocations[instructions.front()] << " -> " << programLocations[instructions.back()] << " ";
        //I.print(roout, true);
//        out << std::endl;

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
            return vReg(registers, F, reg->getName().str(), v);
        }
        if(auto v = dyn_cast<Argument>(reg)) {
            Function& F = *v->getParent();
            return vReg(registers, F, "Arg" + reg->getName().str(), v);
        }
        out.reportError("Value not an instruction or argument: ");
        roout << *reg << "\n";
        roout << *reg->getType() << "\n";
        roout.flush();
        assert(0);
        return nullptr;
    }

    Value* vGetMemOffset(GenerationContext* gctx, Value* registers, Value* reg) {

        Value* v;
        if(isa<Instruction>(reg) || isa<Argument>(reg)) {
            v = vReg(registers, reg);
            v = builder.CreateLoad(v);
        } else {
            v = vMap(gctx, reg);
        }
        return builder.CreatePtrToInt(v, t_intptr, "modelpointer");
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
        auto regs = builder.CreateBitOrPointerCast(registers, PointerType::get(registerLayout[&F].registerLayout, 0),
                                                   F.getName().str() + "_registers"
        );
        auto iIdx = valueRegisterIndex.find(reg);
        assert(iIdx != valueRegisterIndex.end());
        auto idx = iIdx->second;
        return vRegDirect(regs, F.getName().str() + "_" + name, ConstantInt::get(t_int, idx));
    }

    Value* vReg(Value* registers, FunctionType* FType, std::string name, Value* reg) {
        std::vector<Type*> args;
        for(unsigned int i=0; i < FType->getNumParams(); ++i) {
            args.push_back(FType->getParamType(i));
        }
        auto rType = StructType::create( ctx, args, "functiontype");
        registers = builder.CreateBitOrPointerCast(registers, rType->getPointerTo(), name + "_registers");
        return vRegDirect(registers, name, reg);
    }

    Value* vRegDirect(Value* registers, std::string name, Value* reg) {
        auto v = builder.CreateGEP( registers
                                  , { ConstantInt::get(t_int, 0)
                                    , reg
                                    }
                                  , name
                                  );

//        auto sz = builder.CreateGEP(registerLayout[&F].registerLayout,
//                                    ConstantPointerNull::get(registerLayout[&F].registerLayout->getPointerTo()),
//                                    {ConstantInt::get(t_int, 1)}
//        );
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("vReg mapped model-register %zu (" + F.getName().str() + ":" + strout.str() + ") to %p (registers start address %p, size %p)\n")
//                                    , ConstantInt::get(t_int, idx)
//                                    , v
//                                    , registers
//                                    , sz
//                            }
//        );
        return v;
    }

    Value* vRegUsingOffset(Value* registers, Value* offsetInBytes, Type* type) {


        /// pointer add!
        auto v = generatePointerAdd(registers, offsetInBytes);

        v = builder.CreatePointerCast(v, type->getPointerTo());

        return v;
    }

    Value* generatePointerToGlobal(Value* start, GlobalVariable* gv) {
        int idx = valueRegisterIndex[gv];
        start = builder.CreatePointerCast(start, t_globals->getPointerTo());
        auto v = builder.CreateGEP( t_globals
                                  , start
                                  , { ConstantInt::get(t_int, 0)
                                    , ConstantInt::get(t_int, idx)
                                    }
        );
        return v;
    }

    Value* generateModelPointerToGlobal(GlobalVariable* gv) {
        int idx = valueRegisterIndex[gv];
        auto v = builder.CreateGEP( t_globals
                                  , ConstantPointerNull::get(t_globals->getPointerTo())
                                  , { ConstantInt::get(t_int, 0)
                                    , ConstantInt::get(t_int, idx)
                                    }
        );
        return makePointer(ConstantInt::get(t_int64, 0), v, v->getType());
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

        if(auto gv = dyn_cast<GlobalVariable>(OI)) {

            assert(!OI->getType()->isStructTy());
            return generateModelPointerToGlobal(dyn_cast<GlobalVariable>(OI));
//            return builder.CreatePointerCast(gvVal, OI->getType());
            if(gv->isConstant()) {
                auto& mappedGV = mappedConstantGlobalVariables[gv];
                if(!mappedGV) {
                    abort();
                }
                return mappedGV;

            } else {
                return generateModelPointerToGlobal(dyn_cast<GlobalVariable>(OI));
//                ChunkMapper cm_memory(gctx, type_memory);
//                auto chunkMemory = lts["memory"].getValue(gctx->svout);
//                auto chunk = cm_memory.generateGet(chunkMemory);
//                auto chunkData = generateChunkGetData(chunk);
//                return generatePointerToGlobal(generatePointerAdd(chunkData, c_globalMemStart), dyn_cast<GlobalVariable>(OI));
            }


//            auto ptr = generatePointerAdd(chunkData, modelPointer);

//            auto idx = valueRegisterIndex[OI];
//            return lts["globals"][idx].getValue(gctx->svout);
//            Value* mem = lts["memory"].getValue(gctx->svout);
//            return builder.CreateLoad(vReg(registers, OI));
//            generatePointerToGlobal(generatePointerAdd(ConstantPointerNull::get(t_globals->getPointerTo()), c_globalMemStart), dyn_cast<GlobalVariable>(OI));

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
            if(auto func = dyn_cast<Function>(OI)) {
//                roout << "FUNCTION VALUE " << *ConstantInt::get(t_int64, programLocations[&*func->getEntryBlock().begin()]) << "\n";
//                roout.flush();
                auto c = ConstantInt::get(t_int64, programLocations[&*func->getEntryBlock().begin()]);
                return builder.CreateIntToPtr(c, func->getFunctionType()->getPointerTo());
            } else {
                return OI;
            }

        // If we have a known mapping, perform the mapping
        } else if(valueRegisterIndex.count(OI) > 0) {
//            assert(!OI->getType()->isStructTy());
            Value* registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            auto load = builder.CreateLoad(vReg(registers, OI));
            load->setAlignment(Align(1));
            return load;

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
            auto OI2 = vMap(gctx, OI);
//            roout << "mapping " << *OI << ", size = " << generateSizeOfNow(OI) << "\n";
//            roout << "     to " << *OI2 << "\n";
            OI = OI2;
        }
    }

    /**
     * @brief Generates the next-state relation for the Instruction @c I
     * This should be done duing generation, as the instruction is inserted
     * at the current insertion point of the builder.
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    Value* generateNextStateForInstructionValueMapped(GenerationContext* gctx, Instruction* I) {

        // Clone the instruction
        Instruction* IC = I->clone();

//        roout << *IC << "\n";
//        roout.flush();

        // Change all the registers used in the instruction to the mapped
        // registers using the vMap() mapping and loading the value from the
        // state-vector. This is because the registers are in the state-vector
        // and thus in-memory, thus they need to be loaded.
        vMapOperands(gctx, IC);

        // Insert the instruction
        builder.Insert(IC);

//        roout << *IC << "\n";
//        roout.flush();

        setDebugLocation(IC, __FILE__, __LINE__);

        // If the instruction has a return value, store the result in the SV
        if(IC->getType() != t_void) {
            assert(valueRegisterIndex[I]);
            auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
            auto store = builder.CreateStore(IC, vReg(registers, I));
            store->setAlignment(Align(1));
            setDebugLocation(store, __FILE__, __LINE__ - 2);
//            std::string str;
//            raw_string_ostream strout(str);
//            strout << "reg_F_" << *IC->getParent()->getParent()
//                   << "_IC_" << *IC;
//            strout.flush();
//            IC->setName(str);
        }
        return nullptr;
    }

    /**
     * @brief Generates the next-state relation for the Instruction @c I
     * This is the general function to call for generating the next-state
     * relation for an instruction.
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    Value* generateNextStateForInstruction(GenerationContext* gctx, Instruction* I) {
//        roout << "Handling instruction: " << *I << "\n";
//        roout.flush();

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
            case Instruction::AtomicCmpXchg:
                return generateNextStateForInstruction(gctx, dyn_cast<AtomicCmpXchgInst>(I));
            case Instruction::Call:
                return generateNextStateForCall(gctx, dyn_cast<CallInst>(I));
            case Instruction::ExtractValue:
                return generateNextStateForInstruction(gctx, dyn_cast<ExtractValueInst>(I));
            case Instruction::GetElementPtr:
                return generateNextStateForInstruction(gctx, dyn_cast<GetElementPtrInst>(I));
            case Instruction::PHI:
            case Instruction::Unreachable:
                return nullptr;
            case Instruction::AtomicRMW:
                return generateNextStateForInstruction(gctx, dyn_cast<AtomicRMWInst>(I));
            case Instruction::Switch:
            case Instruction::IndirectBr:
            case Instruction::Invoke:
            case Instruction::Resume:
            case Instruction::CleanupRet:
            case Instruction::CatchRet:
            case Instruction::CatchSwitch:
            case Instruction::Fence:
            case Instruction::CleanupPad:
            case Instruction::VAArg:
            case Instruction::ShuffleVector:
            case Instruction::LandingPad:
                roout << "Unsupported instruction: " << *I << "\n";
                roout.flush();
                assert(0);
                return nullptr;
            case Instruction::InsertValue:
            case Instruction::ExtractElement:
            case Instruction::InsertElement:
            default:
                break;
        }

        // The default behaviour
        return generateNextStateForInstructionValueMapped(gctx, I);
    }

    Value* generateNextStateForInstruction(GenerationContext* gctx, AtomicRMWInst* I) {
        assert(I);
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* ptr = vGetMemOffset(gctx, registers, I->getPointerOperand());
        Value* val = vMap(gctx, I->getValOperand());
        Value* newVal = nullptr;
        Type* type = I->getValOperand()->getType();

        auto storagePtr = generateAccessToMemory(gctx, ptr, type);
        storagePtr->setName("storagePtr");
        auto oldVal = builder.CreateLoad(storagePtr);
        oldVal->setAlignment(Align(1));
        oldVal->setName("oldVal");

        switch(I->getOperation()) {
            /// *p = v
            case AtomicRMWInst::BinOp::Xchg:
                newVal = val;
                break;
            /// *p = old + v
            case AtomicRMWInst::BinOp::Add:
                newVal = builder.CreateAdd(oldVal, val);
                break;
            /// *p = old - v
            case AtomicRMWInst::BinOp::Sub:
                newVal = builder.CreateSub(oldVal, val);
                break;
            /// *p = old & v
            case AtomicRMWInst::BinOp::And:
                newVal = builder.CreateAnd(oldVal, val);
                break;
            /// *p = ~(old & v)
            case AtomicRMWInst::BinOp::Nand:
                newVal = builder.CreateAdd(oldVal, val);
                newVal = builder.CreateNot(newVal);
                break;
            /// *p = old | v
            case AtomicRMWInst::BinOp::Or:
                newVal = builder.CreateOr(oldVal, val);
                break;
            /// *p = old ^ v
            case AtomicRMWInst::BinOp::Xor:
                newVal = builder.CreateXor(oldVal, val);
                break;
            /// *p = old >signed v ? old : v
            case AtomicRMWInst::BinOp::Max:
                newVal = builder.CreateSelect(builder.CreateICmpSLT(oldVal, val), val, oldVal);
                break;
            /// *p = old <signed v ? old : v
            case AtomicRMWInst::BinOp::Min:
                newVal = builder.CreateSelect(builder.CreateICmpSLT(oldVal, val), oldVal, val);
                break;
            /// *p = old >unsigned v ? old : v
            case AtomicRMWInst::BinOp::UMax:
                newVal = builder.CreateSelect(builder.CreateICmpULT(oldVal, val), val, oldVal);
                break;
            /// *p = old <unsigned v ? old : v
            case AtomicRMWInst::BinOp::UMin:
                newVal = builder.CreateSelect(builder.CreateICmpULT(oldVal, val), oldVal, val);
                break;
            /// *p = old + v
            case AtomicRMWInst::BinOp::FAdd:
                newVal = builder.CreateFAdd(oldVal, val);
                break;
            /// *p = old - v
            case AtomicRMWInst::BinOp::FSub:
                newVal = builder.CreateFSub(oldVal, val);
                break;
            default:
                abort();
        }
        generateStore(gctx, ptr, newVal, type);
        return oldVal;
    }

    Value* generateNextStateForInstruction(GenerationContext* gctx, AtomicCmpXchgInst* I) {
        assert(I);
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* ptr = vGetMemOffset(gctx, registers, I->getPointerOperand());
        Value* expected = vMap(gctx, I->getCompareOperand());
//        Value* expected_ptr = vGetMemOffset(gctx, registers, I->getCompareOperand());
        expected->setName("expected");
        Value* desired = vMap(gctx, I->getNewValOperand());
        desired->setName("desired");
        Type* type = I->getNewValOperand()->getType();

        auto storagePtr = generateAccessToMemory(gctx, ptr, type);
        storagePtr->setName("storagePtr");
        auto loadedPtr = builder.CreateLoad(storagePtr);
        loadedPtr->setAlignment(Align(1));
        loadedPtr->setName("loaded_ptr");

        auto cmp = builder.CreateICmpEQ(loadedPtr, expected);

        llvmgen::If genIf(builder, "cmpxchg_impl");

        Value* resultReg = vReg(registers, I);
        auto resultRegValue = builder.CreateGEP(I->getType(), resultReg, {ConstantInt::get(t_int, 0), ConstantInt::get(t_int, 0)});
        auto resultRegSuccess = builder.CreateGEP(I->getType(), resultReg, {ConstantInt::get(t_int, 0), ConstantInt::get(t_int, 1)});

        builder.CreateStore(loadedPtr, resultRegValue)->setAlignment(Align(1));
        genIf.setCond(cmp);
        auto bbTrue = genIf.getTrue();
        auto bbFalse = genIf.getFalse();
        genIf.generate();

        builder.SetInsertPoint(&*bbTrue->getFirstInsertionPt());
        generateStore(gctx, ptr, desired, type);
        builder.CreateStore(ConstantInt::get(t_bool, 1), resultRegSuccess)->setAlignment(Align(1));

//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("[memory inst] cmpxchg \033[1m%zx\033[0m %u==%u %u: success\n")
//                                    , ptr
//                                    , loadedPtr
//                                    , expected
//                                    , desired
//                           }
//        );

        builder.SetInsertPoint(&*bbFalse->getFirstInsertionPt());
//        generateStore(gctx, expected_ptr, loadedPtr, type);
        builder.CreateStore(ConstantInt::get(t_bool, 0), resultRegSuccess)->setAlignment(Align(1));

//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("cmpxchg \033[1m%zx\033[0m %u!=%u %u: failure\n")
//                                    , ptr
//                                    , loadedPtr
//                                    , expected
//                                    , desired
//                            }
//        );

        builder.SetInsertPoint(genIf.getFinal());
        return nullptr;

    }

    Value* generateNextStateForInstruction(GenerationContext* gctx, GetElementPtrInst* I) {
        return generateNextStateForInstructionValueMapped(gctx, I);
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//        auto v = vReg(registers, I);
//            builder.CreateCall( pins("printf")
//                    , { generateGlobalString("GetElementPtrInst[%u] %p\n")
//                                        , ConstantInt::get(t_int, gctx->thread_id)
//                                        , vMap(gctx, I->getPointerOperand())
//                                }
//            );
//        for(size_t idx = 1; idx < I->getNumOperands(); ++idx) {
//            builder.CreateCall( pins("printf")
//                    , { generateGlobalString(" @ %u\n")
//                                        , vMap(gctx, I->getOperand(idx))
//                                });
//        }
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("-> %p (%p)\n")
//                                    , builder.CreateLoad(v)
//                                    , v
//                            });
    }

    Value* generateNextStateForInstruction(GenerationContext* gctx, ExtractValueInst* I) {
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        auto arg = vReg(registers, I->getAggregateOperand());

        std::vector<Value*> idxs;

        idxs.push_back(ConstantInt::get(t_int, 0));

        for(auto& i: I->getIndices()) {
            idxs.push_back(ConstantInt::get(t_int, i));
        }

        auto select = builder.CreateGEP(I->getAggregateOperand()->getType(), arg, idxs);
        auto v = builder.CreateLoad(select);
        v->setAlignment(Align(1));
        builder.CreateStore(v, vReg(registers, I))->setAlignment(Align(1));

        auto offset = builder.CreatePtrToInt(select, t_int);
        offset = builder.CreateSub(offset, builder.CreatePtrToInt(registers, t_int));

//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("extract from %p: %p (%u) -> %p (registers = %p, offset = %u)\n")
//                                    , arg
//                                    , select
//                                    , v
//                                    , vReg(registers, I)
//                                    , registers
//                                    , offset
//                            });
        return nullptr;
    }

    Value* generateConditionForInstruction(GenerationContext* gctx, Instruction* I) {

        // Handle the instruction more specifically
        switch(I->getOpcode()) {
            case Instruction::Alloca:
            case Instruction::Load:
            case Instruction::Store:
            case Instruction::Ret:
            case Instruction::Br:
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
            case Instruction::ExtractValue:
            case Instruction::InsertValue:
                return nullptr;
            case Instruction::Call:
                return generateConditionForCall(gctx, dyn_cast<CallInst>(I));
            case Instruction::VAArg:
            case Instruction::ExtractElement:
            case Instruction::InsertElement:
            case Instruction::ShuffleVector:
            case Instruction::LandingPad:
                roout << "Unsupported instruction: " << *I << "\n";
                assert(0);
                return nullptr;
            default:
                break;
        }

        // The default behaviour
        return nullptr;
    }

    Value* generateConditionForCall(GenerationContext* gctx, CallInst* I) {
        assert(I);

        // If this is inline assembly
        if(I->isInlineAsm()) {
            auto inlineAsm = dyn_cast<InlineAsm>(I->getCalledOperand());
            out.reportNote("Found ASM: " + inlineAsm->getAsmString());
            assert(0);
        } else {

            // If this is a declaration, there is no body within the LLVM model,
            // thus we need to handle it differently.
            Function* F = I->getCalledFunction();
            if(!F) {
            } else if(F->isDeclaration()) {

//                auto it = gctx->gen->hookedFunctions.find(F->getName().str());

                // If this is a hooked function
//                if(it != gctx->gen->hookedFunctions.end()) {
//                } else
                if(F->isIntrinsic()) {
                } else if(F->getName().equals("__atomic_load")) {
                } else if(F->getName().equals("__atomic_store")) {
                } else if(F->getName().equals("__atomic_compare_exchange")) {
                } else if(F->getName().equals("malloc")) { // __LLMCOS_Object_New
                } else if(F->getName().equals("__LLMCOS_Object_CheckAccess")) {
                } else if(F->getName().equals("__LLMCOS_Object_CheckNotOverlapping")) {
                } else if(F->getName().equals("__LLMCOS_Fatal")) {
                    Value* p_status = lts["status"].getValue(gctx->svout);
                    builder.CreateStore(ConstantInt::get(t_int, GlobalStatus::ENDED_FAILURE), p_status);
                } else if(F->getName().equals("__LLMCOS_Warning")) {
                } else if(F->getName().equals("__assert_fail")) {
                } else if(F->getName().equals("pthread_create")) { // __LLMCOS_Thread_New
                } else if(F->getName().equals("pthread_join")) { // __LLMCOS_Thread_Join
                    // int pthread_join(pthread_t thread, void **value_ptr);


                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

                    auto elementLength = generateSizeOf(type_threadresults_element->getLLVMType());
                    auto result = builder.CreateAlloca(type_threadresults_element->getLLVMType());

                    auto tres_p = lts["tres"].getValue(gctx->svout);
                    auto tresStateID = builder.CreateLoad(tres_p);
                    auto tid_p = vReg(registers, I->getArgOperand(0));
                    auto tid = builder.CreateLoad(tid_p, t_intp, "tid");
                    auto offset = builder.CreateMul(tid, elementLength);

                    StateManager sm_tres(gctx->userContext, this, type_threadresults);
                    sm_tres.downloadPartBytes(tresStateID, offset, elementLength, result);

                    auto tres_element_status_p = (*root_threadresults_element)["status"].getValue(result);
                    auto tres_element_status = builder.CreateLoad(tres_element_status_p);
                    return builder.CreateICmpNE(tres_element_status, ConstantInt::get(tres_element_status->getType(), 0));

                } else if(llmcvm_func(F->getName().str())) {
                } else {
                    out.reportError("Unhandled function call for condition generation: " + F->getName().str());
                }

                // Otherwise, there is an LLVM body available, so it is modeled.
            } else {
            }
        }
        return nullptr;
    }

    /**
     * @brief Generates the next-state relation for the ReturnInst @c I
     * @param gctx GenerationContext
     * @param I The instruction to generate the next-state relation of
     */
    Value* generateNextStateForInstruction(GenerationContext* gctx, ReturnInst* I) {
        stack.popStackFrame(gctx, I);
        gctx->alteredPC = true;
        return nullptr;
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
    Value* generateNextStateForInstruction(GenerationContext* gctx, AllocaInst* I) {
        Value* size = generateAlignedSizeOf(I->getAllocatedType());
        Value* ptr = generateAllocateMemory(gctx, size);
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* ret = vReg(registers, I);
        ptr = gctx->gen->builder.CreateIntToPtr(ptr, I->getType());
        auto store = gctx->gen->builder.CreateStore(ptr, ret);
        setDebugLocation(store, __FILE__, __LINE__);
        // Debug
//        builder.CreateCall(pins("printf"), {generateGlobalString("Allocated %u bytes at %zx\n"), size, ptr});
//        assert(I);
//
//        Heap heap(gctx, gctx->svout);
//
//        // Perform the malloc
//        auto ptr = gctx->gen->builder.CreateIntCast(heap.malloc(I), t_intptr, false);
//
//        // [ 8 bits ][  16 bits  ][    24 bit    ]
//        //  threadID  origin PC?   actual pointer
//
//        auto shiftedThreadID = gctx->gen->builder.CreateShl(ConstantInt::get(gctx->gen->t_int64, gctx->thread_id), ptr_offset_threadid);
//        ptr = gctx->gen->builder.CreateOr(ptr, shiftedThreadID);
//
//        heap.upload();
//
//        // Store the pointer in the model-register
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//        Value* ret = vReg(registers, I);
//        ptr = gctx->gen->builder.CreateIntToPtr(ptr, I->getType());
//        auto store = gctx->gen->builder.CreateStore(ptr, ret);
//        setDebugLocation(store, __FILE__, __LINE__);
//
//        // Debug
////        builder.CreateCall(pins("printf"), {generateGlobalString("stored %i to %p\n"), ptr, ret});
//
//        // Action label
//        std::string str;
//        raw_string_ostream strout(str);
//        strout << "r[" << valueRegisterIndex[I] << "] " << *I;
//
//        ChunkMapper cm_action(gctx, type_action);
//        Value* actionLabelChunkID = cm_action.generatePut( ConstantInt::get(t_int, (strout.str().length())&~3) //TODO: this cuts off a few characters
//                                                         , generateGlobalString(strout.str())
//                                                         );
//        gctx->edgeLabels["action"] = actionLabelChunkID;
//
//// attempt to report only one transition
////        auto r = builder.CreateCall(f_pins_getnextall, {gctx->model, gctx->svout, gctx->cb, gctx->userContext});
////        builder.CreateRet(r);
////        BasicBlock* main_start  = BasicBlock::Create(ctx, "unreachable" , builder.GetInsertBlock()->getParent());
////        builder.SetInsertPoint(main_start);

        return nullptr;
    }

    DIBuilder* getDebugBuilder() {
        static DIBuilder* dib = nullptr;
        if(!dib) {
            dib = new DIBuilder(*dmcModule);
        }
        return dib;
    }

    DICompileUnit* getCompileUnit() {
        static DICompileUnit* cu = nullptr;
        if(!cu) {
#if LLVM_VERSION_MAJOR < 4
            cu = getDebugBuilder()->createCompileUnit( dwarf::DW_LANG_hi_user
                                                      , dmcModule->getName(), ".", "llmc", false, "", 0);
#else
            auto file = getDebugBuilder()->createFile(dmcModule->getName(), ".");
            cu = getDebugBuilder()->createCompileUnit(dwarf::DW_LANG_C, file, "llmc", 0, "", 0);
#endif
        }
        return cu;
    }

    DIScope* getDebugScopeForSourceFile(DIScope* functionScope, std::string const& fileName) {
//        return functionScope;
        auto file = getDebugBuilder()->createFile(fileName, ".");
        return DILexicalBlockFile::get(functionScope->getContext(), functionScope, file, 0);
    }

    DISubprogram* getDebugScopeForFunction(Function* F) {
        assert(F);
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
        I->setDebugLoc(DILocation::get( ctx, linenr
                                      , 1
                                      , getDebugScopeForSourceFile(getDebugScopeForFunction(I->getParent()->getParent()), file)
        ));
    }

    void setDebugLocation(Value* V, std::string const& file, int linenr) {
        Instruction* I = dyn_cast<Instruction>(V);
        if(I) {
            setDebugLocation(I, file, linenr);
        }
    }

//    Value* getChunkPartOfPointer(Value* v) {
//        v = builder.CreatePtrToInt(v, t_intptr);
//        v = builder.CreateLShr(v, 32);
//        return v;
//    }
//
//    Value* getOffsetPartOfPointer(Value* v) {
//        v = builder.CreatePtrToInt(v, t_intptr);
//        v = builder.CreateBinOp(Instruction::BinaryOps::And, v, ConstantInt::get(t_intptr, 0xFFFFFFFFULL, false));
//        return v;
//    }

    Value* getLengthOfStateID(Value* v) {
        v = builder.CreateLShr(v, 40);
        return v;
    }

    Value* getCreatorProcessorIDOfPointer(Value* v) {
        v = builder.CreatePtrToInt(v, t_intptr);
        v = builder.CreateLShr(v, 56);
        v = builder.CreateSub(v, ConstantInt::get(v->getType(), 1));
        v->setName("modelpointer_processID");
        return v;
    }

    Value* getOffsetPartOfPointer(Value* v) {
        v = builder.CreatePtrToInt(v, t_intptr);
        v = builder.CreateBinOp(Instruction::BinaryOps::And, v, ConstantInt::get(t_intptr, 0xFFFFFFFFFFFFFFULL, false));
        v->setName("modelpointer_offset");
        return v;
    }

    Value* makePointer(Value* processorID, Value* offset, Type* type) {
        if(offset->getType()->isPointerTy()) {
            offset = builder.CreatePtrToInt(offset, t_intptr);
        } else {
            offset = builder.CreateIntCast(offset, t_intptr, false);
        }
        processorID = builder.CreateIntCast(processorID, t_intptr, false);
        processorID = builder.CreateAdd(processorID, ConstantInt::get(t_intptr, 1));
        processorID = builder.CreateShl(processorID, 56);
//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("processorID=%zx offset=%zx\n"), processorID, offset
//                            }
//        );
        if(type->isPointerTy()) {
            return builder.CreateIntToPtr(builder.CreateOr(processorID, offset), type);
        } else {
            return builder.CreateOr(processorID, offset);
        }
    }

    Value* generateAccessToMemory(GenerationContext* gctx, Value* modelPointer, Value* size) {
        if(!modelPointer->getType()->isIntegerTy()) {
            std::string str;
            raw_string_ostream ros(str);
            ros << *modelPointer;
            ros.flush();
            out.reportError("internal: need integer as pointer into memory: " + str);
        }
        StateManager sm_memory(gctx->userContext, this, type_memory);
        auto processorID = getCreatorProcessorIDOfPointer(modelPointer);
        auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
        chunkMemory->setName("chunkMemory");
        auto storage = addAlloca(gctx->gen->t_char, builder.GetInsertBlock()->getParent(), size);
        auto offset = getOffsetPartOfPointer(modelPointer);
        sm_memory.downloadPartBytes(chunkMemory, offset, size, storage);
        return storage;

//        auto chunk = cm_memory.generateGet(chunkMemory);
//        auto chunkData = generateChunkGetData(chunk);
//        auto ptr = generatePointerAdd(chunkData, modelPointer);
//        return builder.CreateLoad(ptr);
    }

    Value* generateAccessToMemory(GenerationContext* gctx, Value* modelPointer, Type* type) {
        auto v = generateAccessToMemory(gctx, modelPointer, generateAlignedSizeOf(type));
        return builder.CreatePointerCast(v, type->getPointerTo());
    }

    Value* generateLoad(GenerationContext* gctx, Value* modelPointer, Type* type) {
//        builder.CreateCall(pins("llmc_memory_check"), {modelPointer});
        auto storage = generateAccessToMemory(gctx, modelPointer, type);
//        auto chunk = cm_memory.generateGet(chunkMemory);
//        auto chunkData = generateChunkGetData(chunk);
//        auto ptr = generatePointerAdd(chunkData, modelPointer);
        auto load = builder.CreateLoad(storage);
        load->setAlignment(Align(1));
        return load;
    }

    void generateStore(GenerationContext* gctx, Value* modelPointer, Value* dataPointerOrRegister, Type* type) {
        StateManager sm_memory(gctx->userContext, this, type_memory);
        auto processorID = getCreatorProcessorIDOfPointer(modelPointer);

//        llvmgen::If2 genIf(builder, builder.CreateICmpEQ(processorID, ConstantInt::get(t_int64, -1, false)), "generatestore_ptrcheck");
//        genIf.startTrue();

//        builder.CreateCall( pins("printf")
//                , { generateGlobalString("Check pointer: %zx\n"), modelPointer
//                            }
//        );
//        genIf.endTrue();
//        genIf.finally();

        auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        if(dataPointerOrRegister->getType() == type) {
            auto v = addAlloca(type, builder.GetInsertBlock()->getParent());
            builder.CreateStore(dataPointerOrRegister, v)->setAlignment(Align(1));
            dataPointerOrRegister = v;
//            builder.CreateCall( pins("printf")
//                    , { generateGlobalString("did an alloca %u\n"), builder.CreateLoad(dataPointerOrRegister)
//                                }
//            );
        }
        assert(dataPointerOrRegister->getType()->isPointerTy());
        if(dataPointerOrRegister->getType()->getPointerElementType() != type) {
            std::string str;
            raw_string_ostream ros(str);
            ros << "\n" << dataPointerOrRegister->getType()->getPointerElementType() << " != " << type;
            ros << "\n" << *dataPointerOrRegister->getType()->getPointerElementType();
            ros << "\n" << *type;
            ros.flush();
            out.reportError("internal: types do not match: " + str);
            abort();
        }
//        builder.CreateCall(pins("llmc_memory_check"), {modelPointer});
        auto offset = getOffsetPartOfPointer(modelPointer);
        auto newMem = sm_memory.deltaBytes(chunkMemory, offset, generateAlignedSizeOf(type), dataPointerOrRegister);
        builder.CreateStore(newMem, chunkMemory)->setAlignment(Align(1));
    }

    void generateStore(GenerationContext* gctx, Value* modelPointer, Value* dataPointerOrRegister, Value* size) {
        StateManager sm_memory(gctx->userContext, this, type_memory);
        auto processorID = getCreatorProcessorIDOfPointer(modelPointer);
        auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//        builder.CreateCall(pins("llmc_memory_check"), {modelPointer});
        auto offset = getOffsetPartOfPointer(modelPointer);
        auto newMem = sm_memory.deltaBytes(chunkMemory, offset, size, dataPointerOrRegister);
        builder.CreateStore(newMem, chunkMemory)->setAlignment(Align(1));
    }

//    void generateNextStateForStoreInstruction(GenerationContext* gctx, StoreInst* I) {
//        Value* ptr = I->getPointerOperand();
//
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//
//        if(isa<GlobalVariable>(ptr) || isa<ConstantExpr>(ptr) || isa<Constant>(ptr)) {
//            Instruction* IC = I->clone();
//            IC->setName("IC");
//            vMapOperands(gctx, IC);
//            builder.Insert(IC);
//            setDebugLocation(IC, __FILE__, __LINE__);
//        } else {
//            ChunkMapper cm_memory(gctx, type_memory);
//            //auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
//            auto chunkMemory = lts["memory"].getValue(gctx->svout);
//            Value* sv_memorylen = nullptr;
//
//            //Value* chunkID = getChunkPartOfPointer(ptr);
//            ptr = builder.CreateLoad(vReg(registers, ptr));
//            Value* offset = getOffsetPartOfPointer(ptr);
//            auto val = I->getValueOperand();
//            if(isa<Argument>(val) || isa<Instruction>(val)) {
//                val = vReg(registers, val);
//            } else {
//                auto valMapped = vMap(gctx, val);
//                //val = builder.CreateAlloca(val->getType());
//                auto alloc = addAlloca(val->getType(), builder.GetInsertBlock()->getParent());
//                val = alloc;
//                setDebugLocation(alloc, __FILE__, __LINE__);
//                auto store = builder.CreateStore(valMapped, val);
//                setDebugLocation(store, __FILE__, __LINE__);
//            }
//            auto newMem = cm_memory.generateCloneAndModify(chunkMemory, offset, val, generateSizeOf(I->getValueOperand()), sv_memorylen);
//            builder.CreateStore(newMem, chunkMemory);
//        }
//    }

    Value* generateAllocateMemory(GenerationContext* gctx, Value* size) {
        Value* processorID = gctx->thread_id;
//        auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
        auto memorySize = lts["processes"][processorID]["msize"].getValue(gctx->svout);
        Value* currentMemorySize = builder.CreateLoad(memorySize);
        Value* newMemorySize = builder.CreateAdd(currentMemorySize, builder.CreateIntCast(size, currentMemorySize->getType(), false));
        builder.CreateStore(newMemorySize, memorySize);
        return makePointer(processorID, currentMemorySize, t_int64);
    }

    void generateNextStateForLoadInstruction(GenerationContext* gctx, LoadInst* I) {
    }

    Value* vMapPointer(GenerationContext* gctx, Value* registers, Value* sv_memorydata, Value* modelPointerValueRegister, Type* type) {
        Value* simPointerValue;
        Value* simPointerValueRegister = nullptr;
        if( isa<ConstantExpr>(modelPointerValueRegister)
         || isa<Constant>(modelPointerValueRegister)
         || isa<GlobalVariable>(modelPointerValueRegister)
          ) {
            roout << "Tried to vMapPointer using a constant or constantexpr:\n" << *modelPointerValueRegister << "\n";
            simPointerValue = vMap(gctx, modelPointerValueRegister);
        } else {
            simPointerValueRegister = vReg(registers, modelPointerValueRegister);
            simPointerValue = builder.CreateLoad(simPointerValueRegister);
        }

        // Load the model-register to obtain the pointer to memory

        // Since the pointer is an offset within the heap, add
        // the start address of the loaded memory chunk to the
        // offset to obtain the current real address. Current
        // means within the scope of this next-state call.
        auto simPointerValue_p = builder.CreatePtrToInt(simPointerValue, t_intptr);
//        auto allocatorID = builder.CreateLShr(simPointerValue_p, ptr_offset_threadid);
        simPointerValue_p = builder.CreateAnd(simPointerValue_p, ptr_mask_location);
        simPointerValue_p = builder.CreateAdd(simPointerValue_p, builder.CreatePtrToInt(sv_memorydata, t_intptr));
        simPointerValue_p = builder.CreateIntToPtr(simPointerValue_p, type);

//        if(simPointerValueRegister) {
//            builder.CreateCall( pins("printf")
//                              , { generateGlobalString("vMapPointer mapped model-pointer %zx to %p (memory start address %p) using register at %zx (register start address %p)\n")
//                                , simPointerValue
//                                , simPointerValue_p
//                                , sv_memorydata
//                                , simPointerValueRegister
//                                , registers
//                                }
//            );
//        } else {
//            builder.CreateCall( pins("printf")
//                              , { generateGlobalString("vMapPointer mapped model-pointer %zx to %p (memory start address %p) (loaded from global)\n")
//                                , simPointerValue
//                                , simPointerValue_p
//                                , sv_memorydata
//                                }
//            );
//        }


        // Debug
        // FIXME: add out-of-bounds check
//                builder.CreateCall(pins("printf"), {generateGlobalString("access @ %x\n"), OI});
        return simPointerValue_p;
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
    Value* generateNextStateForMemoryInstruction(GenerationContext* gctx, LoadInst* I) {
        Value* ptr = I->getPointerOperand();
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* modelPtr = vGetMemOffset(gctx, registers, ptr);
        auto loadedValue = generateLoad(gctx, modelPtr, I->getType());
        builder.CreateStore(loadedValue, vReg(registers, I))->setAlignment(Align(1));
        return nullptr;
    }
    Value* generateNextStateForMemoryInstruction(GenerationContext* gctx, StoreInst* I) {
        Value* ptr = I->getPointerOperand();
        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
        Value* modelPtr = vGetMemOffset(gctx, registers, ptr);
        auto mappedValue = vMap(gctx, I->getValueOperand());
        generateStore(gctx, modelPtr, mappedValue, I->getValueOperand()->getType());
        return nullptr;
    }

    //    template<typename T>
//    void generateNextStateForMemoryInstruction(GenerationContext* gctx, T* I) {
//        assert(I);
//        Value* ptr = I->getPointerOperand();
//
//        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//
//        // Load the memory by making a copy
//        ChunkMapper cm_memory(gctx, type_memory);
////        auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
//        auto chunkMemory = lts["memory"].getValue(gctx->svout);
//        Value* sv_memorylen;
//        auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);
////        builder.CreateCall(pins("printf"), {generateGlobalString("Allocated %i bytes\n"), sv_memorylen});
//
////        roout << "generateNextStateForMemoryInstruction " << *I << "\n";
//
//        // Clone the memory instruction
//        Instruction* IC = I->clone();
//        if(isa<LoadInst>(IC)) dyn_cast<LoadInst>(IC)->setAlignment(MaybeAlign(1));
//        if(isa<StoreInst>(IC)) dyn_cast<StoreInst>(IC)->setAlignment(MaybeAlign(1));
////        if(!IC->getType()->isVoidTy()) {
////            IC->setName("IC");
////        }
//
////        Value* mappedPointer = nullptr;
////        Value* mappedValue = nullptr;
//
//        // Value-map all the operands of the instruction
//        for(auto& OI: IC->operands()) {
//
//            // If this is the pointer operand to a global, a different mapping
//            // needs be performed
//            if(dyn_cast<Value>(OI.get()) == ptr) {// && !isa<GlobalVariable>(ptr) && !isa<ConstantExpr>(ptr) && !isa<Constant>(ptr)) {
//
//                OI = vMapPointer(gctx, registers, sv_memorydata, OI.get(), I->getPointerOperand()->getType());
//
//            // If this is a 'normal' register or global, simply map it
//            } else {
//                OI = vMap(gctx, OI);
//            }
//
//        }
//
//        // Insert the cloned instruction
//        builder.Insert(IC);
//
//        if(IC->getType() != t_void) {
//            LoadInst* loadInst = dyn_cast<LoadInst>(IC);
//            Value* memOffset = vGetMemOffset(gctx, registers, I->getPointerOperand());
//            std::string s;
//            raw_string_ostream ros(s);
//            assert(I);
//            ros << *I;
////            if(loadInst->getType()->isIntegerTy()) {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Loading integer \033[1m%zx\033[0m (%zu) from memory location \033[1m%zx\033[0m: %s\n"),
////                                                    loadInst, loadInst, memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            } else if(loadInst->getType()->isPointerTy()) {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Loading pointer \033[1m%p\033[0m from memory location \033[1m%zx\033[0m: %s\n"),
////                                                    loadInst, memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            } else {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Loading some value from memory location \033[1m%zx\033[0m: %s\n"),
////                                                    memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            }
//        } else {
//            StoreInst* storeInst = dyn_cast<StoreInst>(IC);
//            Value* memOffset = vGetMemOffset(gctx, registers, I->getPointerOperand());
//            std::string s;
//            raw_string_ostream ros(s);
//            ros << *I;
////            if(storeInst->getValueOperand()->getType()->isIntegerTy()) {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Storing integer \033[1m%zx\033[0m (%zu) to memory location \033[1m%zx\033[0m: %s\n"),
////                                                    storeInst->getValueOperand(), storeInst->getValueOperand(),
////                                                    memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            } else if(storeInst->getValueOperand()->getType()->isPointerTy()) {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Storing pointer \033[1m%p\033[0m to memory location \033[1m%zx\033[0m: %s\n"),
////                                                    storeInst->getValueOperand(),
////                                                    memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            } else {
////                builder.CreateCall(pins("printf"), {generateGlobalString(
////                        "[memory inst] Storing some value to memory location \033[1m%zx\033[0m: %s\n"),
////                                                    memOffset, generateGlobalString(ros.str())
////                                   }
////                );
////            }
//        }
//
//        //        setDebugLocation(IC, __FILE__, __LINE__ - 1);
//
//        // If the instruction has a return value, store the result in the SV
//        if(IC->getType() != t_void) {
//            if(valueRegisterIndex.count(I) == 0) {
//                roout << *I << " (" << I << ")" << "\n";
//                roout.flush();
//                assert(0 && "instruction not indexed");
//            }
//            builder.CreateStore(IC, vReg(registers, I))->setAlignment(MaybeAlign(1));
////            builder.CreateCall( pins("printf")
////                              , {generateGlobalString("Setting register %i to %i")
////                                , ConstantInt::get(t_int, valueRegisterIndex[I])
////                                , IC
////                                }
////                              );
//        }
//
////        if(debugChecks) {
////            if(IC->getOpcode() == Instruction::Store) {
////                // Make sure register remain the same
////                auto registers_src = lts["processes"][gctx->thread_id]["r"].getValue(gctx->src);
////                createMemAssert(registers_src, registers, t_registers_max_size);
////            } else if(IC->getOpcode() == Instruction::Load) {
////                // Make sure globals are the same
////                auto globals = lts["globals"].getValue(gctx->svout);
////                auto globals_src = lts["globals"].getValue(gctx->src);
////                createMemAssert(globals_src, globals, generateSizeOf(lts["globals"].getLLVMType()));
////            } else {
////                assert(0);
////            }
////        }
//
//        // Upload the new memory chunk
//        auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
//        builder.CreateStore(newMem, chunkMemory);
//    }

    void createMemAssert(Value* a, Value* b, Value* size) {
        a = builder.CreatePointerCast(a, t_voidp);
        b = builder.CreatePointerCast(b, t_voidp);
        size = builder.CreateIntCast(size, t_int64, false);
        Instruction* cmp = builder.CreateCall(llmcvm_func("__LLMCOS_memcmp"), {a, b, size});
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
    Value* generateNextStateForCall(GenerationContext* gctx, CallInst* I) {
        assert(I);

        Value* condition = nullptr;

        // If this is inline assembly
        if(I->isInlineAsm()) {
            auto inlineAsm = dyn_cast<InlineAsm>(I->getCalledOperand());
            do {
                if(inlineAsm->getConstraintString() == "~{memory},~{dirflag},~{fpsr},~{flags}") {
                    if(inlineAsm->getAsmString() == "") {
                        // Compiler barrier, simply ignore the statement
                        continue;
                    }
                    if(inlineAsm->getAsmString() == "mfence") {
                        // Memory barrier, need to add support
                        continue;
                    }
                }
                out.reportNote("Found unsupported ASM: " + inlineAsm->getAsmString() + " | " + inlineAsm->getConstraintString());
                assert(0);
                return nullptr;
            } while(false);
        } else {

            // If this is a declaration, there is no body within the LLVM model,
            // thus we need to handle it differently.
            Function* F = I->getCalledFunction();
            if(!F) {
                string ftype;
                raw_string_ostream ftype_stream(ftype);
                ftype_stream << *I->getFunctionType();
//                out.reportNote("Handling function call to unknown function " + ftype_stream.str());
                auto fValue = vMap(gctx, I->getCalledOperand());
                fValue = builder.CreatePtrToInt(fValue, t_intptr);

                builder.CreateCall( pins("printf")
                        , { generateGlobalString("CALLING FUNCTION %u\n")
                                            , fValue
                                    }
                );

                std::vector<Value*> args;
                for(unsigned int i=0; i < I->getNumArgOperands(); ++i) {
                    args.push_back(vMap(gctx, I->getArgOperand(i)));
                }
                stack.pushStackFrame(gctx, I->getFunctionType(), fValue, args, I);
            } else if(F->isDeclaration()) {

//                auto it = gctx->gen->hookedFunctions.find(F->getName().str());

//                out.reportNote("Handling declaration " + F->getName().str());

//                bool pointerMapFirstOperand = true;
//                bool pointerMapAllOther = true;
//                if(F->getName().startswith_lower("printf")) {
//                    pointerMapAllOther = false;
//                }

                // If this is a hooked function
                if(false) { //it != gctx->gen->hookedFunctions.end()) {

//                    // Print
//                    out.reportNote("Handling " + F->getName().str());
//
//                    assert(&I->getContext() == &F->getContext());
//
//                    // Load the entire memory (copy)
////                    ChunkMapper cm_memory(gctx, type_memory);
////                    auto chunkMemory = lts["memory"].getValue(gctx->svout);
////                    Value* sv_memorylen;
////                    auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);
//
//                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//
//                    // Map the operands
//                    std::vector<Value*> args;
//                    std::vector<std::pair<Value*,Value*>> memories;
//                    for(unsigned int i = 0; i < I->getNumArgOperands(); ++i) {
//                        auto Arg = I->getArgOperand(i);
//
//                        // TODO: when having multiple heaps, this needs to look into the pointer to determine which memory to copy
//                        if(((i == 0 && pointerMapFirstOperand) || (i > 0 && pointerMapAllOther)) && Arg->getType()->isPointerTy()) { //TODO: temp for printf
//                            Arg = vGetMemOffset(gctx, registers, Arg);
//                            auto processorID = getCreatorProcessorIDOfPointer(Arg);
//                            Arg = getOffsetPartOfPointer(Arg);
//
//                            // Load the entire memory (copy)
//                            ChunkMapper cm_memory(gctx, type_memory);
//                            auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
//                            Value* sv_memorylen;
//                            auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);
//                            memories.emplace_back(sv_memorydata, sv_memorylen);
//
//                            Arg = generatePointerAdd(sv_memorydata, Arg);
//                        } else {
//                            Arg = vMap(gctx, Arg);
//                            memories.emplace_back(nullptr, nullptr);
//                        }
//                        args.push_back(Arg);
//                    }
//
//                    auto call = gctx->gen->builder.CreateCall(it->second, args);
//                    setDebugLocation(call, __FILE__, __LINE__-1);
//
//
//                    for(unsigned int i = 0; i < I->getNumArgOperands(); ++i) {
//                        auto Arg = I->getArgOperand(i);
//                        if(((i == 0 && pointerMapFirstOperand) || (i > 0 && pointerMapAllOther)) && Arg->getType()->isPointerTy()) { //TODO: temp for printf
//                            Arg = vGetMemOffset(gctx, registers, Arg);
//                            auto processorID = getCreatorProcessorIDOfPointer(Arg);
//                            Arg = getOffsetPartOfPointer(Arg);
//                            ChunkMapper cm_memory(gctx, type_memory);
//                            auto chunkMemory = lts["processes"][processorID]["m"].getValue(gctx->svout);
//                            Value*& sv_memorydata = memories[i].first;
//                            Value*& sv_memorylen = memories[i].second;
//                            auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
//                            builder.CreateStore(newMem, chunkMemory);
//                        }
//                    }

                } else if(F->isIntrinsic()) {
//                    out.reportNote("Handling intrinsic " + F->getName().str());

                    // TODO: we can use pushStackFrame to simulate a real memcpy

                    if(F->getName().startswith("llvm.memcpy")) {
                        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
                        Value* dst = vGetMemOffset(gctx, registers, I->getArgOperand(0));
                        Value* src = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        Value* size = vMap(gctx, I->getArgOperand(2));
//                        Value* isVolatile = vMap(gctx, I->getArgOperand(3));
                        auto storage = generateAccessToMemory(gctx, src, size); //TODO: align using src/dst?
                        generateStore(gctx, dst, storage, size);
                    }
                } else if(F->getName().equals("__atomic_load")) {

                    // void __atomic_load (type *ptr, type *ret, int memorder)
                    // Basically this does *ret = *ptr;
                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

                    Value* size;
                    Value* ptr;
                    Value* ret;
                    Type* type;
                    Value* memorder;

                    if(F->getFunctionType()->getNumParams() == 3) {
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(0));
                        ret = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        type = dyn_cast<PointerType>(I->getArgOperand(0)->getType())->getPointerElementType();
                        memorder = I->getArgOperand(2);
                        size = generateAlignedSizeOf(type);
                    } else {
                        size = vMap(gctx, I->getArgOperand(0));
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        ret = vGetMemOffset(gctx, registers, I->getArgOperand(2));
                        type = dyn_cast<PointerType>(I->getArgOperand(1)->getType())->getPointerElementType();
                        memorder = I->getArgOperand(3);
                    }

                    (void)memorder;

//                    builder.CreateCall( pins("printf")
//                            , { generateGlobalString("__atomic_load %p %p %u\n")
//                                                , ptr
//                                                , ret
//                                                , size
//                                        }
//                    );

                    auto storage = generateAccessToMemory(gctx, ptr, size);
                    generateStore(gctx, ret, storage, size);
//
//                    Type* theType;
//                    if()
//
//                    ChunkMapper cm_memory(gctx, type_memory);
////                    auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
//                    auto chunkMemory = lts["memory"].getValue(gctx->svout);
//                    Value* newLength;
//                    auto newMem = cm_memory.generateCloneAndModify(chunkMemory, vGetMemOffset(gctx, registers, I->getArgOperand(1)), storeResult, generateSizeOf(t_voidp), newLength);
//                    builder.CreateStore(newMem, chunkMemory);

                    // void __atomic_store (type *ptr, type *val, int memorder)
                    // __atomic_compare_exchange (type *ptr, type *expected, type *desired, bool weak, int success_memorder, int failure_memorder)

                } else if(F->getName().equals("__atomic_store")) {

                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
                    // void __atomic_store (type *ptr, type *val, int memorder)
                    // Basically this does *ptr = *val;

                    Value* size;
                    Value* ptr;
                    Value* val;
                    Type* type;
                    Value* memorder;

                    if(F->getFunctionType()->getNumParams() == 3) {
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(0));
                        val = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        type = dyn_cast<PointerType>(I->getArgOperand(0)->getType())->getPointerElementType();
                        memorder = I->getArgOperand(2);
                        size = generateAlignedSizeOf(type);
                    } else {
                        size = vMap(gctx, I->getArgOperand(0));
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        val = vGetMemOffset(gctx, registers, I->getArgOperand(2));
                        type = dyn_cast<PointerType>(I->getArgOperand(1)->getType())->getPointerElementType();
                        memorder = I->getArgOperand(3);
                    }

                    (void)memorder;

//                    builder.CreateCall( pins("printf")
//                            , { generateGlobalString("__atomic_store %p %p %u\n")
//                                                , ptr
//                                                , val
//                                                , size
//                                        }
//                    );

                    auto storage = generateAccessToMemory(gctx, val, size);
                    generateStore(gctx, ptr, storage, size);

                } else if(F->getName().equals("__atomic_compare_exchange")) {

                    // bool __atomic_compare_exchange(size_t size, void *ptr, void *expected, void *desired,            int success_order,    int failure_order   )
                    // bool __atomic_compare_exchange(             type *ptr, type *expected, type *desired, bool weak, int success_memorder, int failure_memorder)

                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

                    Value* size;
                    Value* ptr;
                    Value* expected;
                    Value* desired;
                    Value* weak;
                    Type* type;

                    if(I->getArgOperand(0)->getType()->isPointerTy()) {
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(0));
                        expected = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        desired = vGetMemOffset(gctx, registers, I->getArgOperand(2));
                        weak = vMap(gctx, I->getArgOperand(3));
                        type = dyn_cast<PointerType>(I->getArgOperand(0)->getType())->getPointerElementType();
                        size = generateAlignedSizeOf(type);
                    } else {
                        size = vMap(gctx, I->getArgOperand(0));
                        ptr = vGetMemOffset(gctx, registers, I->getArgOperand(1));
                        expected = vGetMemOffset(gctx, registers, I->getArgOperand(2));
                        desired = vGetMemOffset(gctx, registers, I->getArgOperand(3));
                        weak = ConstantInt::get(t_bool, 0);
                        type = dyn_cast<PointerType>(I->getArgOperand(1)->getType())->getPointerElementType();
                    }

                    (void)weak;

//                    Value* memorder_success = vMap(gctx, I->getArgOperand(4));
//                    Value* memorder_failure = vMap(gctx, I->getArgOperand(5));

                    auto storagePtr = generateAccessToMemory(gctx, ptr, size);
                    auto storageExpected = generateAccessToMemory(gctx, expected, size);
                    auto storageDesired = generateAccessToMemory(gctx, desired, size);

//                    auto loadedPtr = builder.CreateLoad(storagePtr);
//                    auto loadedExpected = builder.CreateLoad(storageExpected);
//                    auto loadedDesired = builder.CreateLoad(storageDesired);

                    Value* resultReg = vReg(registers, I);
//                    auto cmp = builder.CreateICmpEQ(loadedPtr, loadedExpected);
                    Value* cmp = builder.CreateCall(llmcvm_func("__LLMCOS_memcmp"), { storagePtr, storageExpected, size});
                    cmp = builder.CreateICmpEQ(cmp, ConstantInt::get(cmp->getType(), 0));

                    llvmgen::If genIf(builder, "__atomic_compare_exchange");

                    genIf.setCond(cmp);
                    auto bbTrue = genIf.getTrue();
                    auto bbFalse = genIf.getFalse();
                    genIf.generate();

                    builder.SetInsertPoint(&*bbTrue->getFirstInsertionPt());

                    generateStore(gctx, ptr, storageDesired, size);
                    builder.CreateStore(ConstantInt::get(t_bool, 1), resultReg);
//                    builder.CreateCall( pins("printf")
//                            , { generateGlobalString("__atomic_compare_exchange %p %p %p %u: success\n")
//                                                , ptr
//                                                , expected
//                                                , desired
//                                                , size
//                                        }
//                    );

                    builder.SetInsertPoint(&*bbFalse->getFirstInsertionPt());

                    generateStore(gctx, expected, storagePtr, size);
                    builder.CreateStore(ConstantInt::get(t_bool, 0), resultReg);
//                    builder.CreateCall( pins("printf")
//                            , { generateGlobalString("__atomic_compare_exchange %p %p %p %u: failure\n")
//                                                , ptr
//                                                , expected
//                                                , desired
//                                                , size
//                                        }
//                    );

                    builder.SetInsertPoint(genIf.getFinal());

                } else if(F->getName().equals("malloc")) { // __LLMCOS_Object_New

                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);
//                    Heap heap(gctx, gctx->svout);
                    Value* bytes = I->getArgOperand(0);
//                    roout << "bytes: " << *bytes << "\n";
//                    roout.flush();
//                    Value* offset = heap.malloc(vMap(gctx, bytes));
//                    heap.upload();
                    Value* offset = generateAllocateMemory(gctx, vMap(gctx, bytes));
//                    auto offsetOld = offset;
                    offset = gctx->gen->builder.CreateIntCast(offset, t_intptr, false);
                    offset = gctx->gen->builder.CreateIntToPtr(offset, I->getType());
//                    builder.CreateCall( pins("printf")
//                            , { generateGlobalString("malloc returning %i %p\n")
//                                                , offsetOld
//                                                , offset
//                                        }
//                    );
                    builder.CreateStore(offset, vReg(registers, I))->setAlignment(Align(1));

                } else if(F->getName().equals("__LLMCOS_Object_CheckAccess")) {
                } else if(F->getName().equals("__LLMCOS_Object_CheckNotOverlapping")) {
                } else if(F->getName().equals("__LLMCOS_Fatal")) {
                } else if(F->getName().equals("__LLMCOS_Warning")) {
                } else if(F->getName().equals("__assert_fail")) {
                } else if(F->getName().equals("pthread_create")) { // __LLMCOS_Thread_New
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

                        if(auto threadFunction = dyn_cast<Function>(I->getArgOperand(2))) {
//                            GenerationContext gctx_copy = *gctx;
//                            gctx_copy.thread_id = ConstantInt::get(t_int, tIdx);
//                            builder.CreateCall( pins("printf")
//                                    , { generateGlobalString("pushStackFrame %p to tid %u\n"), vMap(gctx, I->getArgOperand(2)), ConstantInt::get(t_int, tIdx)
//                                                }
//                            );
                            stack.pushStackFrame(gctx, *threadFunction, {I->getArgOperand(3)}, nullptr,
                                                 ConstantInt::get(t_int, tIdx));
                        } else if(I->getArgOperand(2)->getType()->isPointerTy() && I->getArgOperand(2)->getType()->getPointerElementType()->isFunctionTy()) {
                            auto fType = dyn_cast<FunctionType>(I->getArgOperand(2)->getType()->getPointerElementType());
                            assert(fType);
                            auto fVal = vMap(gctx, I->getArgOperand(2));
                            fVal = builder.CreatePtrToInt(fVal, t_intptr);
                            stack.pushStackFrame(gctx, fType, fVal, {vMap(gctx, I->getArgOperand(3))}, nullptr,
                                                 ConstantInt::get(t_int, tIdx));
                        } else {
                            abort();
                        }

                        // Update thread ID and threadsStarted
                        auto tid_p = lts["processes"][tIdx]["tid"].getValue(gctx->svout);
                        Value* threadsStarted = builder.CreateLoad(threadsStarted_p);
                        threadsStarted = builder.CreateAdd(threadsStarted, ConstantInt::get(t_int, 1));
                        builder.CreateStore(threadsStarted, tid_p);
                        builder.CreateStore(threadsStarted, threadsStarted_p);

                        // Load the memory by making a copy
//                        ChunkMapper cm_memory(gctx, type_memory);
//                        auto chunkMemory = lts["processes"][gctx->thread_id]["memory"].getValue(gctx->svout);
//                        auto chunkMemory = lts["memory"].getValue(gctx->svout);
//                        Value* sv_memorylen;
                        //auto sv_memorydata = cm_memory.generateGetAndCopy(chunkMemory, sv_memorylen);

                        // Store the thread ID in the program (pthread_t)
                        auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

//                        auto tid_p_in_program = vMapPointer(gctx, registers, sv_memorydata, I->getArgOperand(0), t_int64p);
//                        builder.CreateStore( builder.CreateIntCast(threadsStarted, t_int64, false)
//                                           , tid_p_in_program
//                                           );

                        Value* modelPointerPID = vGetMemOffset(gctx, registers, I->getArgOperand(0));
                        generateStore(gctx, modelPointerPID, builder.CreateIntCast(threadsStarted, t_int64, false), t_int64);

//                        builder.CreateCall( pins("printf")
//                                          , { generateGlobalString("Started new thread with ID %u, written to %zx\n")
//                                            , threadsStarted
//                                            , modelPointerPID
//                                            }
//                        );

                        // Upload the new memory chunk
                        //auto newMem = cm_memory.generatePut(sv_memorylen, sv_memorydata);
                        //builder.CreateStore(newMem, chunkMemory);

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
                } else if(F->getName().equals("pthread_join")) { // __LLMCOS_Thread_Join
                    // int pthread_join(pthread_t thread, void **value_ptr);

                    auto registers = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

                    // Return 0
                    auto returnRegister = vReg(registers, *I->getParent()->getParent(), "pthread_join_return_register", I);
                    builder.CreateStore(ConstantInt::get(I->getFunctionType()->getReturnType(), 0), returnRegister);

                    auto elementLength = generateSizeOf(type_threadresults_element->getLLVMType());
                    auto result = builder.CreateAlloca(type_threadresults_element->getLLVMType());

                    auto tres_p = lts["tres"].getValue(gctx->svout);
                    auto tresStateID = builder.CreateLoad(tres_p);
                    auto tid_p = vReg(registers, I->getArgOperand(0));
                    auto tid = builder.CreateLoad(tid_p, t_intp, "tid");
                    auto offset = builder.CreateMul(builder.CreateIntCast(tid, elementLength->getType(), false), elementLength);

                    StateManager sm_tres(gctx->userContext, this, type_threadresults);
                    sm_tres.downloadPartBytes(tresStateID, offset, elementLength, result);

                    auto tres_element_status_p = (*root_threadresults_element)["status"].getValue(result);
                    auto tres_element_status = builder.CreateLoad(tres_element_status_p);
                    auto tres_element_value_p = (*root_threadresults_element)["value"].getValue(result);
                    auto tres_element_value = builder.CreateLoad(tres_element_value_p);

                    // CAM the thread result into the pointer argument given to pthread_join
                    auto resultPointer = vGetMemOffset(gctx, registers, I->getArgOperand(1));

                    condition = builder.CreateICmpNE(tres_element_status, ConstantInt::get(tres_element_status->getType(), 0));

                    auto pthread_join_enabled = BasicBlock::Create(ctx, "pthread_join_enabled", builder.GetInsertBlock()->getParent());
                    assert(gctx->noReportBB);
                    builder.CreateCondBr(condition, pthread_join_enabled, gctx->noReportBB);

                    builder.SetInsertPoint(pthread_join_enabled);

                    auto cmp = builder.CreateICmpNE(resultPointer, ConstantInt::get(resultPointer->getType(), 0));
                    llvmgen::If2 genIf(builder, cmp, "pthread_join_has_return_address");

                    genIf.startTrue();

                    generateStore(gctx, resultPointer, tres_element_value, t_voidp);

                    genIf.endTrue();
                    genIf.finally();
//                    builder.CreateCall(pins("llmc_memory_check"), {resultPointer});
//                    ChunkMapper cm_memory(gctx, type_memory);
//                    auto chunkMemory = lts["memory"].getValue(gctx->svout);
//                    Value* newLength;
//                    auto newMem = cm_memory.generateCloneAndModify(chunkMemory, resultPointer, storeResult, generateAlignedSizeOf(t_voidp), newLength);
//                    builder.CreateStore(newMem, chunkMemory);

//                    builder.SetInsertPoint(genIf.getFinal());
                    // Generate an if for whether or not the specific thread is finished or not
//                    llvmgen::If genIf(builder, "pthread_join_result_not_found");
//                    genIf.setCond(builder.CreateICmpEQ(found, ConstantInt::get(found->getType(), 0)));
//                    auto BBTrue = genIf.getTrue();
//                    genIf.generate();
//
//                    // If it is not found, inhibit the transition report
//                    builder.SetInsertPoint(&*BBTrue->getFirstInsertionPt());
//                    builder.CreateStore(ConstantInt::get(t_int, programLocations[I]), dst_pc);
//
//                    builder.SetInsertPoint(genIf.getFinal());

    //                type_threadresults

                    // True case
    //                auto BBTrue = genIf.getTrue();
    //                builder.SetInsertPoint(BBTrue);
    //
    //                auto writeT   hreadResultTo = I->getArgOperand(1);
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

                } else if(llmcvm_func(F->getName().str())) {
                } else {
                    out.reportError("Unhandled function call: " + F->getName().str());
                }

            // Otherwise, there is an LLVM body available, so it is modeled.
            } else {
//                out.reportNote("Handling function call to " + F->getName().str());
                std::vector<Value*> args;
                for(unsigned int i=0; i < I->getNumArgOperands(); ++i) {
                    args.push_back(I->getArgOperand(i));
                }
                stack.pushStackFrame(gctx, *F, args, I);
            }
            gctx->alteredPC = true; //TODO: some calls (intrinsics) do not modify PC
        }
        return condition;
    }

    void generateStoresForPHINodes(GenerationContext* gctx, BasicBlock* from, BasicBlock* to) {
        BasicBlock* const& currentBlock = from;
        auto dst_reg = lts["processes"][gctx->thread_id]["r"].getValue(gctx->svout);

        std::unordered_map<PHINode*, std::vector<PHINode*>> dependencies;

        // For each basic block, go through all PHI nodes to add them
        for(auto& phiNode: to->phis()) {
            dependencies[&phiNode] = {};
        }

        // Then go through all PHI nodes to determine dependencies
        for(auto& phiNode: to->phis()) {

//            roout << "phiNode: " << phiNode << "\n";
//            roout.flush();
            // For each PHI node, check if it depends on a PHI node within the current block
            auto phiValue = dyn_cast<PHINode>(phiNode.getIncomingValueForBlock(currentBlock));
            if(phiValue) {
//                roout << "  phiValue: " << *phiValue << "\n";
//                roout.flush();
            }
            if(phiValue) {
                dependencies[&phiNode].push_back(phiValue);
//                roout << "Adding dependency " << phiNode << " on " << *phiValue << "\n";
//                roout.flush();
            }
        }

//        builder.CreateCall(pins("printf"), {generateGlobalString(
//                "--------------------\n")}
//        );

        int done = 0;
        do {
            done = 0;
            auto it = dependencies.begin();
            while(it != dependencies.end()) {
                PHINode* phiNode = it->first;
                bool hasDepender = false;
                for(auto& it2: dependencies) {
                    if(std::find(it2.second.begin(), it2.second.end(), phiNode) != it2.second.end()) {
//                        roout << "!!!!!!! Detected that " << *it2.first << " depends on " << *phiNode << "\n";
//                        roout.flush();
                        hasDepender = true;
                        break;
                    }
                }
                if(hasDepender) {
                    ++it;
                } else {

                    // Assign the value to the PHI node
                    // associated with the origin-branch
                    auto phiValue = phiNode->getIncomingValueForBlock(currentBlock);
                    auto phiRegister = vReg(dst_reg, phiNode);
                    auto c = builder.CreateStore(vMap(gctx, phiValue), phiRegister);
                    setDebugLocation(c, __FILE__, __LINE__);

                    string sPhiValue;
                    raw_string_ostream rosPhiValue(sPhiValue);
                    rosPhiValue << *phiValue;

                    string sPhiNode;
                    raw_string_ostream rosPhiNode(sPhiNode);
                    rosPhiNode << *phiNode;

                    string sPhiRegister;
                    raw_string_ostream rosPhiRegister(sPhiRegister);
                    rosPhiRegister << *phiRegister;

//                    if(c->getValueOperand()->getType()->isIntegerTy()) {
//                        builder.CreateCall(pins("printf"), {generateGlobalString(
//                                "[phi inst] Copying integer \033[1m%zx\033[0m (%zu)\n"
//                                "           from %s\n"
//                                "           to   %s\n"),
//                                                            c->getValueOperand(), c->getValueOperand(),
//                                                            generateGlobalString(rosPhiValue.str()),
//                                                            generateGlobalString(rosPhiNode.str())
//                                           }
//                        );
//                    } else if(c->getValueOperand()->getType()->isPointerTy()) {
//                        builder.CreateCall(pins("printf"), {generateGlobalString(
//                                "[phi inst] Copying integer \033[1m%p\033[0m\n"
//                                "           from %s\n"
//                                "           to   %s\n"),
//                                                            c->getValueOperand(),
//                                                            generateGlobalString(rosPhiValue.str()),
//                                                            generateGlobalString(rosPhiNode.str())
//                                           }
//                        );
//                    } else {
//                        builder.CreateCall(pins("printf"), {generateGlobalString(
//                                "[phi inst] Copying some value\n"
//                                "           from %s\n"
//                                "           to   %s\n"),
//                                                            generateGlobalString(rosPhiValue.str()),
//                                                            generateGlobalString(rosPhiNode.str())
//                                           }
//                        );
//                    }

                    ++done;
                    it = dependencies.erase(it);
                }
            }
        } while(done > 0);
        assert(dependencies.size() == 0);
    }

    Value* generateNextStateForInstruction(GenerationContext* gctx, BranchInst* I) {
        auto dst_pc = lts["processes"][gctx->thread_id]["pc"].getValue(gctx->svout);

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
                llvmgen::If genIf(builder, "branch_if");
                genIf.setCond(vMap(gctx, condition));
                auto bbTrue = genIf.getTrue();
                auto bbFalse = genIf.getFalse();
                genIf.generate();

                // We absolutely need to implement a run-time handling of PHI nodes, instead of writing to them all


                string s;
                raw_string_ostream ros(s);
                ros << *I;

                builder.SetInsertPoint(&*bbTrue->getFirstInsertionPt());
//                builder.CreateCall(pins("printf"), { generateGlobalString("[branch inst] Branching to TRUE case: %s\n")
//                                           , generateGlobalString(ros.str())
//                                   }
//                );
                generateStoresForPHINodes(gctx, I->getParent(), I->getSuccessor(0));

                builder.SetInsertPoint(&*bbFalse->getFirstInsertionPt());
//                builder.CreateCall(pins("printf"), { generateGlobalString("[branch inst] Branching to FALSE case: %s\n")
//                                           , generateGlobalString(ros.str())
//                                   }
//                );
                generateStoresForPHINodes(gctx, I->getParent(), I->getSuccessor(1));

                builder.SetInsertPoint(genIf.getFinal());
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
                generateStoresForPHINodes(gctx, I->getParent(), I->getSuccessor(0));
            } else {
                roout << "Unconditional branch has more or less than 1 successor: " << *I << "\n";
                roout.flush();
            }
        }

        return nullptr;
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

        // Generate the model initialization function
        // void pins_model_init(model_t)
        f_dmc_initialstate = Function::Create( t_dmc_initialstate
                                                , GlobalValue::LinkageTypes::ExternalLinkage
                                                , "dmc_initialstate", dmcModule
                                                );
        { // pins_model_init

            // Get the argument (the model)
            auto args = f_dmc_initialstate->arg_begin();
            Argument* model = &*args++;

            // Create some space for chunks
            BasicBlock* entry  = BasicBlock::Create(ctx, "entry", f_dmc_initialstate);
            builder.SetInsertPoint(entry);
//            auto sv_memory_init_data = builder.CreateAlloca(t_chunkid);

            // Initialize the initial state
            {

                GenerationContext gctx = {};
                gctx.gen = this;
                gctx.model = model;
                gctx.userContext = model;
                gctx.src = nullptr;
                gctx.svout = s_statevector;
                gctx.alteredPC = false;

                // Init to 0
                builder.CreateMemSet( s_statevector
                                    , ConstantInt::get(t_int8, 0)
                                    , t_statevector_size
                                    , s_statevector->getPointerAlignment(dmcModule->getDataLayout())
                                    );

                // Init stack
//                builder.CreateCall(pins("printf"), {generateGlobalString("Setting up initial stack\n")});
                for(size_t threadID = 0; threadID < MAX_THREADS; ++threadID) {
                    auto pStackChunkID = lts["processes"][threadID]["stk"].getValue(s_statevector);
                    builder.CreateStore(stack.getEmptyStack(&gctx), pStackChunkID);
                }

                auto threadsStarted_p = lts["threadsStarted"].getValue(gctx.svout);
                builder.CreateStore(ConstantInt::get(t_int, 1), threadsStarted_p);

//                StateManager sm_tres(gctx.userContext, this);
//
//                ChunkMapper cm_tres = ChunkMapper(&gctx, type_threadresults);
//                auto tres_p = lts["tres"].getValue(s_statevector);
//                auto t_tres = StructType::get(ctx, {t_int64, t_voidp}, true);
//                auto newTres = builder.CreateAlloca(t_tres);
//                auto len = generateSizeOf(t_tres);
//                builder.CreateMemSet( newTres
//                                    , ConstantInt::get(t_int8, 0)
//                                    , len
//                                    , MaybeAlign(1)
//                                    );
//
//                auto newTresChunk = cm_tres.generatePut(len, newTres);
//                builder.CreateStore(newTresChunk, tres_p);

                // Make sure the empty stack is at chunkID 0
//                ChunkMapper cm_stack(*this, model, type_stack);
//                cm_stack.generatePutAt(ConstantInt::get(t_int, 4), ConstantPointerNull::get(t_intp), 0);

                // Set the initialization value for the globals
//                builder.CreateCall(pins("printf"), {generateGlobalString("Setting up globals\n")});
                Value* globalsInit = addAlloca(t_globals, builder.GetInsertBlock()->getParent());
                builder.CreateMemSet( globalsInit
                        , ConstantInt::get(t_int8, 0)
                        , generateAlignedSizeOf(t_globals)
                        , globalsInit->getPointerAlignment(dmcModule->getDataLayout())
                );
                for(auto& v: module->getGlobalList()) {
                    if(v.hasInitializer()) {
//                        int idx = valueRegisterIndex[&v];
//                        auto& node = lts["globals"][idx];
//                        assert(node.getChildren().size() == 0);
//                        builder.CreateStore(v.getInitializer(), node.getValue(s_statevector));
//                        if(v.getInitializer()->isZeroValue()) {
//                            continue;
//                        }
                        if(v.getName().equals("llvm.global_ctors")) {
                            continue;
                        }
                        assert(mappedConstantGlobalVariables[&v]);

                        auto global_ptr = generatePointerToGlobal(globalsInit, &v);
                        //builder.CreateStore(vMap(&gctx, v.getInitializer()), global_ptr);
                        auto sizze = generateSizeOf(v.getInitializer()->getType());
                        generateBoundCheck(global_ptr, sizze, globalsInit, generateSizeOf(t_globals));
                        builder.CreateMemCpy( global_ptr
                                , global_ptr->getPointerAlignment(dmcModule->getDataLayout())
                                , mappedConstantGlobalVariables[&v]
                                , mappedConstantGlobalVariables[&v]->getPointerAlignment(dmcModule->getDataLayout())
                                , sizze
                        );
                    }
                }

                auto globalsSize = generateAlignedSizeOf(t_globals);

//                builder.CreateCall(pins("printf"), {generateGlobalString("Storing globals into memory, size %u\n"), globalsSize});
//                ChunkMapper cm_memory(&gctx, type_memory);
//                auto newMemory = cm_memory.generatePut(globalsSize, globalsInit);
                StateManager sm_memory(gctx.userContext, this, type_memory);
                auto newMemory = sm_memory.uploadBytes(globalsInit, globalsSize);
                builder.CreateStore(newMemory, lts["processes"][0]["m"].getValue(s_statevector));
                auto memorySize = lts["processes"][0]["msize"].getValue(s_statevector);
                builder.CreateStore(globalsSize, memorySize);

                StateManager sm_root(gctx.userContext, this, lts.getRootType());
                sm_root.uploadBytes(s_statevector, t_statevector_size);
            }

//            // Set the initial state
//            builder.CreateCall(pins("printf"), {generateGlobalString("Calling GBsetInitialState\n")});
//            auto initStateType = pins("GBsetInitialState")->getFunctionType()->getParamType(1);
//            auto initState = builder.CreatePointerCast(s_statevector, initStateType);
//            builder.CreateCall(pins("GBsetInitialState"), {model, initState});

            // Set the next-state ALL function
//            auto next_method_black_t = pins_type("next_method_black_t");
//            assert(next_method_black_t);
//            builder.CreateCall( pins("GBsetNextStateAll")
//                              , {model, builder.CreatePointerCast(f_pins_getnextall2, next_method_black_t)}
//                              );

//            // Set the next-state function
//            auto t_next_method_grey = pins_type("next_method_grey_t");
//            assert(t_next_method_grey);
//            builder.CreateCall( pins("GBsetNextStateLong")
//                    , {model, builder.CreatePointerCast(f_pins_getnext, t_next_method_grey)}
//            );


            builder.CreateRet(ConstantInt::get(t_int64, 1));
        }

    }

    void generateBoundCheck(Value* ptr, Value* size, Value* range_start, Value* range_size) {
        auto f = pins("__LLMCOS_CheckRangeIsInRange");
        builder.CreateCall(f, {
                builder.CreatePointerCast(ptr, f->getFunctionType()->getParamType(0))
            , builder.CreateIntCast(size, f->getFunctionType()->getParamType(1), false)
            , builder.CreatePointerCast(range_start, f->getFunctionType()->getParamType(2))
            , builder.CreateIntCast(range_size, f->getFunctionType()->getParamType(3), false)
        });
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
//            printf("Added node %p %s\n", node, node->getName().c_str());
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

            printf("Handling %s\n", node->getName().c_str());

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
            llvmgen::IRStringBuilder<LLDMCModelGenerator> irs(*this, 256);
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
            auto ts = builder.CreateCall(pins(typeno_setter), {ltstype, slotv, v_typeno});
            setDebugLocation(ts, __FILE__, __LINE__- 1);
            auto ns = builder.CreateCall(pins(name_setter), {ltstype, slotv, name});
            setDebugLocation(ns, __FILE__, __LINE__ - 1);

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
        auto root_size = generateAlignedSizeOf(root.getLLVMType());
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
        printf("lts.getSV()\n");
        lts.getSV().executeLeaves([this, &leavesSV](SVTree* node) {
            leavesSV.push_back(node);
        });
        if(lts.hasLabels()) {
            printf("lts.getLabels()\n");
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
            printf("lts.getEdgeLabels()\n");
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

//        // Loop over the types in the LTS to make them known to LTSmin
//        for(auto& nameType: lts.getTypes()) {
//            auto nameV = generateGlobalString(nameType->_name);
//            auto& t = pinsTypes[nameType->_name];
//            t = builder.CreateCall(pins("lts_type_add_type"), {ltstype, nameV, ConstantPointerNull::get(p)});
//            builder.CreateCall( pins("lts_type_set_format")
//                              , {ltstype, t, ConstantInt::get(t_int, nameType->_ltsminType)}
//                              );
//            builder.CreateCall(pins("printf"), {generateGlobalString("Added type %i: %s\n"), t, nameV});
//        }

//        builder.CreateCall( pins("printf")
//                          , { generateGlobalString("t_statevector_size = %i\n")
//                            , t_statevector_size
//                            }
//                          );
//        builder.CreateCall( pins("printf")
//                          , { generateGlobalString("t_statevector_count = %i\n")
//                            , t_statevector_count
//                            }
//                          );

        // Generate the lts_*() calls to set the types for the state vector
        generateLTSTypeCallsForSVTree( ltstype
                                     , pinsTypes
                                     , F
                                     , lts.getSV()
                                     , "lts_type_set_state_typeno"
                                     , "lts_type_set_state_name"
                                     );

        auto edge_label_root_size = generateAlignedSizeOf(lts.getEdgeLabels().getLLVMType());
        auto edge_label_root_count = ConstantExpr::getUDiv(edge_label_root_size, ConstantInt::get(t_int, 4));
        // Generate the lts_*() calls to set the types for the edge labels
        builder.CreateCall( pins("lts_type_set_edge_label_count")
                          , { ltstype
                            , edge_label_root_count
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
        auto f = dmcModule->getFunction(name);
        if(!f) {
            out.reportError("Could not link to LLMC function: " + name);
            abort();
        }
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
    Function* pins(std::string name, bool abortWhenNotAvailable = true) const {
        auto f = dmcModule->getFunction(name);
        if(!f && abortWhenNotAvailable) {
            out.reportError("Could not link to PINS function: " + name);
            abort();
        }
        return f;
    }

    Function* dmcapi_func(std::string name, bool abortWhenNotAvailable = true) const {
        auto f = up_dmcapiHeaderModule->getFunction(name);
        if(!f && abortWhenNotAvailable) {
            out.reportError("Could not link to DMC API function: " + name);
            abort();
        }
        return f;
    }

    Function* dmcapi_inject_func(std::string name) const {
        auto f = dmcapi_func(name, true);
        auto f2 = Function::Create(f->getFunctionType(), GlobalValue::LinkageTypes::ExternalLinkage, f->getName(), dmcModule);
        return f2;
    }


    Function* llmcvm_func(std::string name, bool abortWhenNotAvailable = false) const {
        auto f = dmcModule->getFunction(name);
        if(!f && abortWhenNotAvailable) {
            out.reportError("Could not link to LLMC VM function: " + name);
            abort();
        }
        return f;
    }

    Type* dmcapi_type(std::string name) const {
        // Get the type from the DMC API header module
        Type* type = dmcModule->getTypeByName(name);

        // If not found, check if there is an alias for it
        if(!type) {
            for(auto& a: dmcModule->getAliasList()) {
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
    AllocaInst* addAlloca(Type* type, Function* f, Value* size = nullptr) {
        IRBuilder<> b(f->getContext());
        b.SetInsertPoint(&*f->getEntryBlock().begin());
        auto v = b.CreateAlloca(type, size);
        v->setAlignment(Align(1));
        b.CreateMemSet( v
                , ConstantInt::get(t_int8, 0)
                , generateAlignedSizeOf(type)
                , v->getPointerAlignment(dmcModule->getDataLayout())
        );
        return v;
    }

    /**
     * @brief Prints the PINS module LLVM IR to the file @c s.
     * @param s The file to write to.
     */
    void writeTo(std::string s) {
        std::error_code EC;
        raw_fd_ostream fdout(s, EC, sys::fs::FA_Read | sys::fs::FA_Write);
        dmcModule->print(fdout, nullptr);
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
            var = new GlobalVariable( *dmcModule
                                    , t
                                    , true
                                    , GlobalValue::LinkageTypes::ExternalLinkage
                                    , ConstantDataArray::getString(ctx, s)
                                    );
            var->setAlignment(Align(4));
        }
        Value* indices[2];
        indices[0] = (ConstantInt::get(t_int, 0, true));
        indices[1] = (ConstantInt::get(t_int, 0, true));
        return folder.CreateGetElementPtr(t, var, indices);
    }

};

} // namespace llmc
