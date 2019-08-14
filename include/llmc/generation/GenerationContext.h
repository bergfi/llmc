#pragma once

#include <unordered_map>

#include <llmc/llvmincludes.h>

using namespace llvm;

namespace llmc {

class LLPinsGenerator;

struct ChuckCacheItem {
    Value* chunkID;
    Value* chunk;
};

/**
 * @class GenerationContext
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Describes the current context while generating LLVM IR.
 * It remembers various details through the generation of a single transition
 * group.
 */
class GenerationContext {
public:

    /**
     * The generator instance
     */
    LLPinsGenerator* gen;

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
     * Pointer to the edgle label array that will be reported to LTSmin
     */
    Value* edgeLabelValue;

    /**
     * Map from an edge label to the value of an edge label
     * Use this to specify the edge label of a transition.
     */
    std::unordered_map<std::string, Value*> edgeLabels;

    std::unordered_map<int, ChuckCacheItem> chunkCache;

    /**
     * The current thread_id
     */
    int thread_id;

    /**
     * Whether the transition group changed the PC
     */
    bool alteredPC;

    Value* cb;
    Value* userContext;

    GenerationContext()
    :   gen(nullptr)
    ,   model(nullptr)
    ,   src(nullptr)
    ,   svout(nullptr)
    ,   edgeLabelValue(nullptr)
    ,   edgeLabels()
    ,   thread_id(0)
    ,   alteredPC(false)
    ,   cb(nullptr)
    ,   userContext(nullptr)
    {
    }
};

} // namespace llmc
