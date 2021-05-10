/*
 * LLMC - LLVM IR Model Checker
 * Copyright © 2013-2021 Freark van der Berg
 *
 * This file is part of LLMC.
 *
 * LLMC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * LLMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LLMC.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

template<typename NODE>
class SVAccessor {
public:
    SVAccessor(NODE& tree): _idx(), _treeOrigin(tree), _treeCurrent(&tree) {
    }

    /**
     * @brief Accesses the child with the specified name
     * @param name The name of the child to access.
     * @return The NODE child node with the specified name.
     */
    SVAccessor<NODE>& operator[](std::string name) {
        for(auto& c: _treeCurrent->getChildren()) {
            if(c->getName() == name) {
                _treeCurrent = c;
                _idx.push_back(llvm::ConstantInt::get(_treeCurrent->gen()->t_int, c->getIndex()));
                return *this;
            }
        }
        std::cerr << "Could not find child `" << name << "' among:";
        for(auto& c: _treeCurrent->getChildren()) {
            std::cerr << " " << c->getName();
        }
        std::cerr << std::endl;
        abort();
    }

    /**
     * @brief Accesses the child at the specified index
     * @param i The index of the child to access.
     * @return The NODE child node at the specified index.
     */
    SVAccessor<NODE>& operator[](size_t i) {
        _treeCurrent = _treeCurrent->getChildren()[i];
        _idx.push_back(llvm::ConstantInt::get(_treeCurrent->gen()->t_int, i));
        return *this;
    }

    SVAccessor<NODE>& operator[](int i) {
        _treeCurrent = _treeCurrent->getChildren()[i];
        _idx.push_back(llvm::ConstantInt::get(_treeCurrent->gen()->t_int, i));
        return *this;
    }

    SVAccessor<NODE>& operator[](llvm::Value* idx) {
        assert(idx->getType()->isIntegerTy());
        idx = _treeOrigin.gen()->builder.CreateIntCast(idx, _treeOrigin.gen()->t_int, false);
        _treeCurrent = _treeCurrent->getChildren()[0]; // This is so terrible, it's not even funny.
        _idx.push_back(idx);
        return *this;
    }

    llvm::Value* getValue(Value* rootValue, Type* desiredType = nullptr) {
        std::vector<Value*> indices;
        std::string name = "svaccessor_";
        NODE* p = &_treeOrigin;

        // Collect all indices to know how to access this variable from
        // the outer structure
        while(p) {
            indices.push_back(ConstantInt::get(_treeOrigin.gen()->t_int, p->getIndex()));
            name = p->getName() + "_" + name;
            p = p->getParent();
        }

        // Reverse the list so it goes from outer to inner
        std::reverse(indices.begin(), indices.end());

        indices.insert(indices.end(), _idx.begin(), _idx.end());

        // Create the GEP
        auto v = _treeOrigin.gen()->builder.CreateGEP(rootValue, ArrayRef<Value*>(indices));

        assert(v);

        // If the real LLVM Type is different from the LLVM Type in the SV,
        // perform a pointer cast, so that the generated code actually uses
        // the real LLVM Type it expects instead of the LLVM Type in the SV.
        // These can differ in the event the type does not fit a number of
        // slots perfectly, thus padding is required.
        if(v->getType() != desiredType && desiredType != nullptr) {
            v = _treeOrigin.gen()->builder.CreatePointerCast(v, PointerType::get(desiredType, 0));
        } else {
//        v->setName(name);
        }
        v->setName(name);
        return v;
    }

private:
    std::vector<llvm::Value*> _idx;
    NODE& _treeOrigin;
    NODE* _treeCurrent;
};

} // namespace llmc