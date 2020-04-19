#pragma once

#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>
#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

class LLPinsGenerator;
class SVTypeManager;
class SVTree;

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
    SVType(std::string name, SVTypeManager* gen)
            : _name(std::move(name)), _ltsminType(LTStypeChunk), _llvmType(nullptr), _PaddedLLVMType(nullptr) {
    }


    SVType(std::string name, data_format_t ltsminType, llvm::Type* llvmType, SVTypeManager* manager);

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

public:
    std::string _name;
    data_format_t _ltsminType;
    int index;
protected:
    llvm::Type* _llvmType;
    llvm::Type* _PaddedLLVMType;
};

class SVStructType : public SVType {
public:
    SVStructType(std::string name, SVTypeManager* manager, std::vector<SVTree*> children);
};

class SVArrayType : public SVType {
public:
    SVArrayType(std::string name, SVType* elementType, SVTypeManager* manager);
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

    SVTree( SVTypeManager* manager
            , std::string name = ""
            , SVType* type = nullptr
            , SVTree* parent = nullptr
            , int index = 0
    )
            :   _manager(manager)
            ,   name(std::move(name))
            ,   typeName(std::move(typeName))
            ,   parent(std::move(parent))
            ,   index(std::move(index))
            ,   type(type)
            ,   llvmType(nullptr)
    {
    }

    LLPinsGenerator* gen();

    SVTypeManager* manager() {
        if(!_manager && parent) {
            _manager = parent->manager();
        }
        return _manager;
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
            :   _manager(nullptr)
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
            :   _manager(nullptr)
            ,   name(std::move(name))
            ,   typeName(std::move(typeName))
            ,   parent(std::move(parent))
            ,   index(0)
            ,   type(type)
            ,   llvmType(nullptr)
    {
        auto st = type ? dyn_cast<StructType>(type->getLLVMType()) : nullptr;
        if(st && st->getNumElements() != children.size()) {
            raw_os_ostream out(std::cerr);
            out << "Number of children does not match:\n";
            out << "LLVM Struct\n";
            for(auto& c: st->elements()) {
                out << "  - " << *c << "\n";
            }
            out << "LLMC Tree children:\n";
            for(auto& c: children) {
                out << "  - " << c->getName() << "\n";
            }
            out.flush();
            assert(0);
        }
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
        children.push_back(new SVTree(manager(), name, nullptr, this, i));
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
            children[i] = new SVTree(manager(), name, nullptr, this, i);
        }
        return *children[i];
    }

    /**
     * @brief Adds a copy of the node @c t as child
     * @param t The node to copy and add.
     * @return A reference to this node
     */
    SVTree& operator<<(SVTree t) {
        printf("Added SVTree %s\n", t.getName().c_str());
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
        printf("Added SVTree child %s\n", t->getName().c_str());
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
    Value* getValue(Value* rootValue);

    Value* getLoadedValue(Value* rootValue);

    /**
     * @brief Returns the number of bytes the variable described by this
     * SVTree node needs in the state-vector.
     * @return The number of bytes needed for the variable.
     */
    Constant* getSizeValue();

    /**
     * @brief Returns the number of state-vector slots the variable
     * described by this SVTree node needs in the state-vector.
     * @return The number of state-vector slots needed for the variable.
     */
    Constant* getSizeValueInSlots();

    SVType* getType() {
        if(type) {
            llvm::StructType* st = dyn_cast<StructType>(type->getLLVMType());
            raw_os_ostream out(std::cerr);
            if(children.size() > 0 && st && st->getNumElements() != children.size()) {
                out << "SVTree " << getName() << " Number of children does not match:\n";
                out << "LLVM Struct\n";
                for(auto& c: st->elements()) {
                    out << "  - " << *c << "\n";
                }
                out << "LLMC Tree children:\n";
                for(auto& c: children) {
                    out << "  - " << c->getName() << "\n";
                }
                out.flush();
                assert(0);
            }
            return type;
        } else {
            type = new SVStructType(name + "_struct", manager(), children);
            llvm::StructType* st = dyn_cast<StructType>(type->getLLVMType());
            assert(st && st->getNumElements() == children.size());
            return type;
        }
    }

    Type* getLLVMType();

    bool isEmpty() {
        return children.size() == 0 && !type;
    }

    bool verify();

    /**
     * @brief Executes @c f for every leaf in the tree.
     */
    template<typename FUNCTION>
    void executeLeaves(FUNCTION &&f) {
//        if(children.size() != 0) {
//            printf("parent %s (%u children) {\n", getName().c_str(), children.size());
//        }
        for(auto& c: children) {
            c->executeLeaves(std::forward<FUNCTION>(f));
        }
        if(children.size() == 0) {
//            printf("child: %p %s\n", this, getName().c_str());
            f(this);
        }
//        if(children.size() != 0) {
//            printf("}\n");
//        }
    }

    size_t count() const {
        return children.size();
    }

    std::vector<SVTree*>& getChildren() {
        return children;
    }

protected:
    SVTypeManager* _manager;
    std::string name;
    std::string typeName;
    SVTree* parent;
    int index;

    SVType* type;
    Type* llvmType;
    std::vector<SVTree*> children;

    friend class LLVMLTSType;

};

} // namespace llmc