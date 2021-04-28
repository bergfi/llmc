#include <llmc/generation/LLVMLTSType.h>
#include <llmc/LLDMCModelGenerator.h>

namespace llmc {

LLDMCModelGenerator* SVTree::gen() {
    return manager()->gen();
}


SVType::SVType(std::string name, Type* llvmType, SVTypeManager* manager)
: _name(std::move(name))
, _llvmType(llvmType)
{

    // Determine if the LLVM Type fits perfectly in a number of state
    // vector slots.
    auto& DL = manager->gen()->dmcModule->getDataLayout();
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

SVStructType::SVStructType(std::string name, SVTypeManager* manager, std::vector<SVTree*> const& children): SVType(name, manager) {
    std::vector<Type*> types;
    for(auto& c: children) {
        types.push_back(c->getLLVMType());
    }

    // Should padded be the same one?
    // TODO: currently, we require packed because we flatten the SV such that LTSmin can cope with it
    // In the future we can send the SV type as tree and tell them where in the flattened version the data is,
    // such that much more information is available even when the SV is dynamically extended
    auto structType = StructType::create(manager->gen()->ctx, types, name, false);
    _PaddedLLVMType = _llvmType = structType;
}

SVStructType::SVStructType(std::string name, SVTypeManager* manager, std::vector<SVType*> const& children): SVType(name, manager) {
    std::vector<Type*> types;
    for(auto& c: children) {
        types.push_back(c->getLLVMType());
    }

    // Should padded be the same one?
    // TODO: currently, we require packed because we flatten the SV such that LTSmin can cope with it
    // In the future we can send the SV type as tree and tell them where in the flattened version the data is,
    // such that much more information is available even when the SV is dynamically extended
    auto structType = StructType::create(manager->gen()->ctx, types, name, false);
    _PaddedLLVMType = _llvmType = structType;
}

SVArrayType::SVArrayType(std::string name, SVTypeManager* manager, SVType* elementType, size_t count): SVType(name, manager) {

    // Should padded be the same one?
    // TODO: currently, we require packed because we flatten the SV such that LTSmin can cope with it
    // In the future we can send the SV type as tree and tell them where in the flattened version the data is,
    // such that much more information is available even when the SV is dynamically extended
    auto arrayType = ArrayType::get(elementType->getLLVMType(), count);
    _PaddedLLVMType = _llvmType = arrayType;
}

Value* SVTree::getValue(Value* rootValue) {
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

    assert(v);

    // If the real LLVM Type is different from the LLVM Type in the SV,
    // perform a pointer cast, so that the generated code actually uses
    // the real LLVM Type it expects instead of the LLVM Type in the SV.
    // These can differ in the event the type does not fit a number of
    // slots perfectly, thus padding is required.
    if(type && type->getLLVMType() != type->getRealLLVMType()) {
        v = gen()->builder.CreatePointerCast(v, PointerType::get(type->getRealLLVMType(), 0), name);
    } else {
//        v->setName(name);
    }
    return v;
}

Value* SVTree::getLoadedValue(Value* rootValue) {
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

Constant* SVTree::getSizeValue() {
    return gen()->generateSizeOf(getLLVMType());
}

Constant* SVTree::getSizeValueInSlots() {
    return ConstantExpr::getUDiv(getSizeValue(), ConstantInt::get(gen()->t_int, 4, true));
}

Type* SVTree::getLLVMType() {
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

bool SVTree::verify() {
    bool r = true;
    for(int i = 0; (size_t)i < children.size(); ++i) {
        auto& c = children[i];
        assert(_isArray || c->getIndex() == i);
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

} // namespace llmc
