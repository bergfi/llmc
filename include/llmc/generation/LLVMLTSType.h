#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <llmc/generation/types/SVTypeManager.h>
#include <llmc/generation/types/SVTree.h>
#include <llmc/generation/types/SVAccessor.h>
#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

class LLPinsGenerator;

/**
 * @class VarRoot
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief An SVTree node describing the root of a tree.
 */
class VarRoot: public SVTree {
public:
    VarRoot(SVTypeManager* manager, std::string name)
    :   SVTree(manager, name)
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
    SVAccessor<SVTree> operator[](std::string name) {
        return (*sv)[name];
    }

    /**
     * @brief Accesses the chil of the root SV node at the specified index
     * @param i The index of the child to access.
     * @return The SVTree child node at the specified index.
     */
    SVAccessor<SVTree> operator[](size_t i) {
        return (*sv)[i];
    }

    SVAccessor<SVTree> operator[](Value* idx) {
        return (*sv)[idx];
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
