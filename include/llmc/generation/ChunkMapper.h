#pragma once

#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>

namespace llmc {

/**
 * @class ChunkMapper
 * @author Freark van der Berg
 * @date 04/07/17
 * @file LLPinsGenerator.h
 * @brief Maps chunkIDs to chunks.
 * Allows easy acces to the chunk system of LTSmin. Construction does not
 * cost anything at runtime.
 */
class ChunkMapper {
private:
    GenerationContext* gctx;
    SVType* type;
    int idx;

    void init();
public:

    /**
     * @brief Constructs a chunk mapper and initializes it.
     * This does not cost any runtime performance.
     * @param gen LLPinsGenerator
     * @param model The LTSmin model
     * @param type The type to use for the chunk mapping.
     */
    ChunkMapper(GenerationContext* gctx, SVType* type)
    : gctx(gctx)
    , type(type)
    , idx(-1)
    {
        assert(type->_ltsminType == LTStypeChunk);
        init();
    }

    /**
     * @brief Returns a read-only chunk reference.
     * @param chunkid The chunkID of the chunk to return
     * @return Read-only chunk reference
     */
    Value* generateGet(Value* chunkid);

    /**
     * @brief Generates the LLVM IR to upload a chunk to LTSmin.
     * @param chunk The chunk to upload
     * @return The chunkID now associated with the uploaded chunk
     */
    Value* generatePut(Value* chunk);

    /**
     * @brief Generates the LLVM IR to upload a chunk to LTSmin.
     * @param len The length of the chunk to upload
     * @param data The data of the chunk to upload
     * @return The chunkID now associated with the uploaded chunk
     */
    Value* generatePut(Value* len, Value* data);

    /**
     * @brief Generates the LLVM IR to upload a chunk to LTSmin.
     * @param len The length of the chunk to upload
     * @param data The data of the chunk to upload
     * @param chunkID the chunkID to associate to the specified chunk
     *
     * NOTE: this can only be used at initialization time, NOT
     * at model-checking time.
     *
     * @todo make check for that
     *
     * @return The chunkID associated with the uploaded chunk
     */
    Value* generatePutAt(Value* len, Value* data, int chunkID);

    /**
     * @brief Gets the chunk specified by @c chunkid and copies it.
     * The copy is done by @c memcpy into a char array alloca'd on the stack.
     * The array Value is returned and the size of the array Value is assigned to @c ch_len.
     * @param chunkid The chunkid of the chunk to get
     * @param ch_len To which the Size of the chunk will be assigned
     * @return Copied chunk
     */
    Value* generateGetAndCopy(Value* chunkid, Value*& ch_len, Value* appendSize = nullptr);

    /**
     * @brief Clones the chunk specified by @c chunkid, with the specified delta.
     * The delta is specified by @c offset, @c data, and @c len, where
     * @c offset and @c len denote where the clone will differ and @c data
     * denotes how the clone will differ.
     *
     *                v------- offset
     *  Original: --------------------
     *                <-len-->
     *  Data:         xxxxxxxx
     *
     *  Clone:    ----xxxxxxxx--------
     *
     *                                    v------- offset
     *  Original: --------------------
     *                                    <-len-->
     *  Data:                             xxxxxxxx
     *
     *  Clone:    --------------------0000xxxxxxxx
     *
     * @param chunkid The chunkID of the chunk to clone
     * @param offset Where the data is to be written in the clone
     * @param data Data to overwrite the clone with
     * @param len Length of @c data
     * @return ChunkID of the cloned chunk
     */
    Value* generateCloneAndModify(Value* chunkid, Value* offset, Value* data, Value* len, Value*& newLength);

    Value* generatePartialGet(Value* chunkid, Value* offset, Value* data, Value* len);

    /**
     * @brief Clone the chunk @c chunkid but with @c data of length @c len
     * appended to it.
     * @param chunkid The chunkID of the chunk to clone
     * @param data The data to append to the clone of the chunk
     * @param len The length of the data to append
     * @return The chunkID of the cloned chunk with the change
     */
    Value* generateCloneAndAppend(Value* chunkid, Value* data, Value* len, Value** oldLength = nullptr);

    /**
     * @brief Clone the chunk @c chunkid but with @c data appended to it.
     * The length of the data is determined by the type of @c data.
     * @param chunkid The chunkID of the chunk to clone
     * @param data The data to append to the clone of the chunk
     * @return The chunkID of the cloned chunk with the change
     */
    Value* generateCloneAndAppend(Value* chunkid, Value* data);

};

} // namespace llmc
