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

#include <unordered_map>

#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

class LLDMCModelGenerator;

struct ChuckCacheItem {
    Value* chunkID;
    Value* chunk;
};

/**
 * @class GenerationContext
 * @author Freark van der Berg
 * @date 04/07/17
 * @brief Describes the current context while generating LLVM IR.
 * It remembers various details through the generation of a single transition
 * group.
 */
class GenerationContext {
public:

    /**
     * The generator instance
     */
    LLDMCModelGenerator* gen;

    /**
     * The LTSmin model
     */
    Value* model;

    /**
     * Pointer to the read-only state-vector of the incoming state
     */
    Value* src;

    /**
     * Pointer to the state-vector that will be reported to LTSmin
     */
    Value* svout;

    /**
     * Map from an edge label to the value of an edge label
     * Use this to specify the edge label of a transition.
     */
    std::unordered_map<std::string, Value*> edgeLabels;

    std::unordered_map<int, ChuckCacheItem> chunkCache;

    /**
     * The current thread_id
     */
    Value* thread_id;

    /**
     * Whether the transition group changed the PC
     */
    bool alteredPC;

    Value* userContext;

    BasicBlock* noReportBB;

    GenerationContext()
    :   gen(nullptr)
    ,   model(nullptr)
    ,   src(nullptr)
    ,   svout(nullptr)
    ,   edgeLabels()
    ,   thread_id(0)
    ,   alteredPC(false)
    ,   userContext(nullptr)
    ,   noReportBB(nullptr)
    {
    }
};

} // namespace llmc
