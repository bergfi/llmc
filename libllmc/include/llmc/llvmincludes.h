#pragma once

#include <cstdio>
#include <cstring>
#include <llvm/Config/llvm-config.h>

#if LLVM_VERSION_MAJOR < 4
#include <llvm/Bitcode/ReaderWriter.h>
#else
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#endif

#include <llvm/Analysis/Passes.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/SymbolTableListTraits.h>
#include <llvm/IR/TypeFinder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include "llvm/Linker/Linker.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Transforms/Scalar.h>
