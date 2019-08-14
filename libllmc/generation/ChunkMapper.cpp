#include <llmc/generation/ChunkMapper.h>
#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>
#include <llmc/LLPinsGenerator.h>

namespace llmc {

void ChunkMapper::init() {
    int index = 0;
    for(auto& t: gctx->gen->lts.getTypes()) {
        if(t->_name == type->_name) {
            idx = index;
            break;
        }
        index++;
    }
}

Value* ChunkMapper::generateGet(Value* chunkid) {
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gctx->gen->builder.CreateLoad(chunkid);
    }
    if(!chunkid->getType()->isIntegerTy()) {
        assert(0);
    }
    return gctx->gen->builder.CreateCall( gctx->gen->pins("pins_chunk_get")
                                        , {gctx->model, ConstantInt::get(gctx->gen->t_int, idx), chunkid}
                                        , "chunk." + type->_name
                                        );
}

Value* ChunkMapper::generatePut(Value* chunk) {
    if(chunk->getType()->isPointerTy()) {
        chunk = gctx->gen->builder.CreateLoad(chunk);
        assert(0 && "this should not work");
    }
    assert(chunk->getType() == gctx->gen->t_chunk);
    return gctx->gen->builder.CreateCall( gctx->gen->pins("pins_chunk_put")
                                        , {gctx->model, ConstantInt::get(gctx->gen->t_int, idx), chunk}
                                        , "chunkid." + type->_name
                                        );
}

Value* ChunkMapper::generatePut(Value* len, Value* data) {
    len = gctx->gen->builder.CreateIntCast(len, gctx->gen->t_chunk->getElementType(0), false);
    data = gctx->gen->builder.CreatePointerCast(data, gctx->gen->t_chunk->getElementType(1));
    Value* chunk = gctx->gen->builder.CreateInsertValue(UndefValue::get(gctx->gen->t_chunk), len, {0});
    chunk = gctx->gen->builder.CreateInsertValue(chunk, data, {1});
    return generatePut(chunk);
}

Value* ChunkMapper::generatePutAt(Value* len, Value* data, int chunkID) {
    assert(isa<IntegerType>(len->getType()));
    assert(isa<PointerType>(data->getType()));
    len = gctx->gen->builder.CreateIntCast(len, gctx->gen->t_chunk->getElementType(0), false);
    data = gctx->gen->builder.CreatePointerCast(data, gctx->gen->t_chunk->getElementType(1));
    Value* chunk = gctx->gen->builder.CreateInsertValue(UndefValue::get(gctx->gen->t_chunk), len, {0});
    chunk = gctx->gen->builder.CreateInsertValue(chunk, data, {1});
    auto vChunkID = ConstantInt::get(gctx->gen->t_int, chunkID);
    gctx->gen->builder.CreateCall( gctx->gen->pins("pins_chunk_put_at")
                                 , {gctx->model, ConstantInt::get(gctx->gen->t_int, idx)
                                   , chunk, vChunkID
                                   }
                                 );
    return vChunkID;
}

Value* ChunkMapper::generateGetAndCopy(Value* chunkid, Value*& ch_len) {
    Value* ch = generateGet(chunkid);
    Value* ch_data = gctx->gen->generateChunkGetData(ch);
    ch_len = gctx->gen->generateChunkGetLen(ch);
    auto copy = gctx->gen->builder.CreateAlloca(gctx->gen->t_int8, ch_len);
    gctx->gen->builder.CreateMemCpy( copy
                                   , copy->getPointerAlignment(gctx->gen->pinsModule->getDataLayout())
                                   , ch_data
                                   , ch_data->getPointerAlignment(gctx->gen->pinsModule->getDataLayout())
                                   , ch_len
                                   );
    return copy;
}

Value* ChunkMapper::generateCloneAndModify(Value* chunkid, Value* offset, Value* data, Value* len, Value*& newLength) {
    //assert(0 && "unknown if the CAM version works");
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gctx->gen->builder.CreateLoad(chunkid);
    }
    newLength = gctx->gen->builder.CreateAdd(gctx->gen->builder.CreateIntCast(offset, gctx->gen->t_int, false), gctx->gen->builder.CreateIntCast(len, gctx->gen->t_int, false));
    auto cmp = gctx->gen->builder.CreateICmpUGE(newLength, len);
    newLength = gctx->gen->builder.CreateSelect(cmp, newLength, len);
    auto f = gctx->gen->pins("pins_chunk_cam");
    return gctx->gen->builder.CreateCall( f
                                 , { gctx->model
                                   , ConstantInt::get(gctx->gen->t_int, idx)
                                   , gctx->gen->builder.CreateIntCast(chunkid, f->getFunctionType()->getParamType(2), false)
                                   , gctx->gen->builder.CreateIntCast(offset, f->getFunctionType()->getParamType(3), false)
                                   , gctx->gen->builder.CreatePointerCast(data, f->getFunctionType()->getParamType(4))
                                   , gctx->gen->builder.CreateIntCast(len, f->getFunctionType()->getParamType(5), false)
                                   }
                                 , "chunkid." + type->_name
                                 );
}

Value* ChunkMapper::generateCloneAndAppend(Value* chunkid, Value* data, Value* len) {
    Value* ch = generateGet(chunkid);
    Value* chData = gctx->gen->generateChunkGetData(ch);
    Value* chLen = gctx->gen->generateChunkGetLen(ch);
    Value* newLength = gctx->gen->builder.CreateAdd(chLen,len);
    auto copy = gctx->gen->builder.CreateAlloca(gctx->gen->t_int8, newLength);
    gctx->gen->builder.CreateMemCpy( copy
                                   , 1
                                   , chData
                                   , 1
                                   , chLen
                                   );
    if(data) {
        gctx->gen->builder.CreateMemCpy( gctx->gen->builder.CreateGEP(copy, {chLen})
                                       , 1
                                       , data
                                       , 1
                                       , len
                                       );
    } else {
        gctx->gen->builder.CreateMemSet( gctx->gen->builder.CreateGEP(copy, {chLen})
                                       , ConstantInt::get(gctx->gen->t_int8, 0)
                                       , len
                                       , 1
                                       );
    }
    return generatePut(newLength, copy);
}

Value* ChunkMapper::generateCloneAndAppend(Value* chunkid, Value* data) {
    Value* ch = generateGet(chunkid);
    Value* chData = gctx->gen->generateChunkGetData(ch);
    Value* chLen = gctx->gen->generateChunkGetLen(ch);

    Value* len = gctx->gen->generateSizeOf(data);
    Value* newLength = gctx->gen->builder.CreateAdd(chLen,len);
    auto copy = gctx->gen->builder.CreateAlloca(gctx->gen->t_int8, newLength);
    gctx->gen->builder.CreateMemCpy( copy
                                   , copy->getPointerAlignment(gctx->gen->pinsModule->getDataLayout())
                                   , chData
                                   , chData->getPointerAlignment(gctx->gen->pinsModule->getDataLayout())
                                   , chLen
                                   );
    auto target = gctx->gen->builder.CreateGEP(copy, {chLen});
    target = gctx->gen->builder.CreatePointerCast(target, data->getType()->getPointerTo());
    gctx->gen->builder.CreateStore(data, target);
    return generatePut(newLength, copy);
}

} // namespace llmc
