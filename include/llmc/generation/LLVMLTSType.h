#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <llmc/llvmincludes.h>
#include <ltsmin/pins.h>
#include <ltsmin/pins-util.h>

using namespace llvm;

namespace llmc {

class LLPinsGenerator;
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
    SVType(std::string name, LLPinsGenerator* gen)
    : _name(std::move(name))
    , _ltsminType(LTStypeChunk)
    , _llvmType(nullptr)
    , _PaddedLLVMType(nullptr) {
    }


    SVType(std::string name, data_format_t ltsminType, Type* llvmType, LLPinsGenerator* gen);

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
protected:
    Type* _llvmType;
    Type* _PaddedLLVMType;
};

class SVStructType: public SVType {
public:
    SVStructType(std::string name, LLPinsGenerator* gen, std::vector<SVTree*> children);
};

class SVArrayType: public SVType {
public:
    SVArrayType(std::string name, SVType* elementType, LLPinsGenerator* gen);
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
            if(st && st->getNumElements() != children.size()) {
                std::cout << "Number of children does not match" << std::endl;
                assert(0);
            }
            return type;
        } else {
            type = new SVStructType(name + "_struct", gen(), children);
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

    friend class LLVMLTSType;

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
 * @class LLVMLTSType
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Class describing the LTS type of the LLVM model,
 */
class LLVMLTSType {
public:

    LLVMLTSType()
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
    LLVMLTSType& operator<<(SVTree* t) {
        (*sv) << t;
        return *this;
    }

    /**
     * @brief Adds a state label node to the LTS type
     * @param t The SVLabel node to add
     * @return *this
     */
    LLVMLTSType& operator<<(SVLabel* t) {
        (*labels) << t;
        return *this;
    }

    /**
     * @brief Adds an edge label node to the LTS type
     * @param t The SVEdgeLabel node to add
     * @return *this
     */
    LLVMLTSType& operator<<(SVEdgeLabel* t) {
        (*edgeLabels) << t;
        return *this;
    }

    /**
     * @brief Sets the root node of the SV tree
     * @param r The root node to set
     * @return *this
     */
    LLVMLTSType& operator<<(SVRoot* r) {
        assert(!sv);
        sv = r;
        return *this;
    }

    /**
     * @brief Sets the root node of the state label tree
     * @param r The root node to set
     * @return *this
     */
    LLVMLTSType& operator<<(SVLabelRoot* r) {
        assert(!labels);
        labels = r;
        return *this;
    }

    /**
     * @brief Sets the root node of the edge label tree
     * @param r The root node to set
     * @return *this
     */
    LLVMLTSType& operator<<(SVEdgeLabelRoot* r) {
        assert(!edgeLabels);
        edgeLabels = r;
        return *this;
    }

    /**
     * @brief Adds the SVType @c t to the list of types in the LTS
     * @param t the SVType to add
     * @return *this
     */
    LLVMLTSType& operator<<(SVType* t) {
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

} // namespace llmc
