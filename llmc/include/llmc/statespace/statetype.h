#pragma once

#include <string>
#include <vector>

namespace llmc { namespace statespace {

class Type {

};

class SingleType: public Type {
public:
    SingleType(const std::string& name) : name(name) {}
private:
    std::string name;
};

class StructType: public Type {
public:

    StructType& addField(std::string name, Type* type) {
        fieldNames.push_back(name);
        fieldTypes.push_back(type);
        return *this;
    }

    const std::vector<std::string>& getFieldNames() const {
        return fieldNames;
    }

    const std::vector<Type*>& getFieldTypes() const {
        return fieldTypes;
    }

private:
    std::vector<std::string> fieldNames;
    std::vector<Type*> fieldTypes;
};

class ArrayType: public Type {
public:
    ArrayType(Type* elementType, size_t elements) : elementType(elementType), elements(elements) {}

    Type* getElementType() const {
        return elementType;
    }

    size_t getElements() const {
        return elements;
    }

private:
    Type* elementType;
    size_t elements;
};

} }