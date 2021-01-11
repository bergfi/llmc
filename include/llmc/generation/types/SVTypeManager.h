#pragma once

#include <llmc/generation/types/SVTree.h>
#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

class LLPinsGenerator;

class SVTypeManager {
public:

    SVTypeManager(LLPinsGenerator* gen): _gen(gen), _types(16) {

    }

    SVType* newType(std::string name) {
        return insertType(new SVType(name, this));
    }

    SVType* newType(std::string name, data_format_t ltsminType, llvm::Type* llvmType) {
        return insertType(new SVType(name, ltsminType, llvmType, this));
    }

    SVType* newStructType(std::string name, std::vector<SVTree*> children) {
        return insertType(new SVStructType(name, this, children));
    }
    SVType* newArrayType(std::string name, SVType* elementType, size_t count) {
        return insertType(new SVArrayType(name, this, elementType, count));
    }

//    SVType* getTypeFor(llvm::Type* llvmType) {
//        for(auto& t: _types) {
//            if(t->getLLVMType() == llvmType) return t;
//        }
//        return newType(llvmType);
//    }

    ~SVTypeManager() {
        for(auto& t: _types) {
            if(t) delete t;
        }
    }

    std::vector<SVType*> const& getTypes() const {
        return _types;
    }

    LLPinsGenerator* gen() const {
        return _gen;
    }

protected:
    SVType* insertType(SVType* type) {
        if(_types.size() >= _types.max_size()) {
            _types.reserve(_types.max_size() * 2);
        }
        _types.emplace_back(type);
        return type;
    }
//    SVType* newType(llvm::Type* llvmType) {
//        switch(llvmType->getTypeID()) {
//            case llvm::Type::TypeID::StructTyID: {
//                auto sType = dyn_cast<llvm::StructType>(llvmType);
//                size_t num = sType->getNumElements();
//                std::vector<SVTree
//                for(size_t i = 0; i < num; ++i) {
//
//                }
//            }
//            case llvm::Type::TypeID::ArrayTyID: {
//                auto aType = dyn_cast<llvm::ArrayType>(llvmType);
//            }
//            case llvm::Type::TypeID::VoidTyID:
//            case llvm::Type::TypeID::HalfTyID:
//            case llvm::Type::TypeID::FloatTyID:
//            case llvm::Type::TypeID::DoubleTyID:
//            case llvm::Type::TypeID::X86_FP80TyID:
//            case llvm::Type::TypeID::FP128TyID:
//            case llvm::Type::TypeID::PPC_FP128TyID:
//            case llvm::Type::TypeID::LabelTyID:
//            case llvm::Type::TypeID::MetadataTyID:
//            case llvm::Type::TypeID::X86_MMXTyID:
//            case llvm::Type::TypeID::TokenTyID:
//            case llvm::Type::TypeID::IntegerTyID:
//            case llvm::Type::TypeID::FunctionTyID:
//            case llvm::Type::TypeID::PointerTyID:
//            case llvm::Type::TypeID::VectorTyID:
//                break;
//        }
//        return insertType(new SVType(name, ltsminType, llvmType, this));
//    }

private:
    LLPinsGenerator* _gen;
    std::vector<SVType*> _types;
};

} // namespace llmc