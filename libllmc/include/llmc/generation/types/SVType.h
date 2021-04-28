#pragma once

#include <llmc/generation/types/SVAccessor.h>

namespace llmc {

class SVTypeManager;
class SVType;
class SVTree;

/**
 * @class SVType
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief A type in the tree of SVTree nodes.
 * @see SVTree
 */
class SVType {
public:
    SVType(std::string name, SVTypeManager* gen)
            : _name(std::move(name)), _llvmType(nullptr), _PaddedLLVMType(nullptr) {
    }


    SVType(std::string name, llvm::Type* llvmType, SVTypeManager* manager);

    /**
     * @brief Request the LLVMType of this SVType.
     * @return The LLVM Type of this SVType
     */
    llvm::Type* getLLVMType() {
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
    llvm::Type* getRealLLVMType() {
        return _llvmType;
    }
//
//    SVTree& operator[](std::string name) {
//        assert(_llvmType->isStructTy());
//        auto s = dyn_cast<StructType>(_llvmType);
//        for(int idx = 0; idx < s->getNumElements(); ++idx) {
//            if(s->getElementType(idx))
//        }
//        return (*sv)[name];
//    }
//
//    SVAccessor<SVType> operator[](Value* idx) {
//        assert(_llvmType->isArrayTy());
//        return SVAccessor<SVType>(*this, idx);
//    }

public:
    std::string _name;
    int index;
protected:
    llvm::Type* _llvmType;
    llvm::Type* _PaddedLLVMType;
};

class SVStructType : public SVType {
public:
    SVStructType(std::string name, SVTypeManager* manager, std::vector<SVTree*> const& children);
    SVStructType(std::string name, SVTypeManager* manager, std::vector<SVType*> const& children);
//private:
//    std::vector<SVTree*> _children;
};

class SVArrayType : public SVType {
public:
    SVArrayType(std::string name, SVTypeManager* manager, SVType* elementType, size_t count);
};

} // namespace llmc