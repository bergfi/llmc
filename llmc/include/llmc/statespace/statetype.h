#pragma once

#include <string>
#include <vector>

namespace llmc { namespace statespace {

class Type {
public:
    virtual void print(std::ostream& os) const = 0;
    friend std::ostream& operator<<(std::ostream& os, Type const& type) {
        type.print(os);
        return os;
    }
};

class SingleType: public Type {
public:
    SingleType(const std::string& name) : name(name) {}
    virtual void print(std::ostream& os) const override {
        os << name;
    }
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

    virtual void print(std::ostream& os) const override {
        os << "{ ";
        for(size_t i = 0; i < fieldNames.size(); ++i) {
            if(i!=0) os << ", ";
            os << fieldNames[i];
            os << ": ";
            os << *fieldTypes[i];
        }
        os << " }";
    }
private:
    std::vector<std::string> fieldNames;
    std::vector<Type*> fieldTypes;
};

class ArrayType: public Type {
public:
    ArrayType(Type* elementType, size_t elements) : _elementType(elementType), _elements(elements) {}

    Type* getElementType() const {
        return _elementType;
    }

    size_t getNumberOfElements() const {
        return _elements;
    }

    virtual void print(std::ostream& os) const override {
        _elementType->print(os);
        os << "[" << _elements << "]";
    }

    bool hasVariableLength() const {
        return _variableLength;
    }

private:
    Type* _elementType;
    size_t _elements;
    bool _variableLength;
};

} }