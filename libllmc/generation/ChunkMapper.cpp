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

#include <llmc/generation/ChunkMapper.h>
#include <llmc/generation/GenerationContext.h>
#include <llmc/generation/LLVMLTSType.h>
#include <llmc/LLDMCModelGenerator.h>

namespace llmc {

void ChunkMapper::init() {
    int index = 0;
    for(auto& t: gen->lts.getTypes()) {
        if(t->_name == type->_name) {
            idx = index;
            break;
        }
        index++;
    }
}

Value* ChunkMapper::generateGet(Value* chunkid) {
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gen->builder.CreateLoad(chunkid);
    }
    if(!chunkid->getType()->isIntegerTy()) {
        assert(0);
    }
    auto call = gen->builder.CreateCall( gen->pins("pins_chunk_get64")
                                             , {userContext, ConstantInt::get(gen->t_int, idx), chunkid}
                                             , "chunk." + type->_name
                                             );
//    gen->setDebugLocation(call, __FILE__, __LINE__ - 4);

    Value* ch_data = gen->generateChunkGetData(call);
    Value* ch_len = gen->generateChunkGetLen(call);
//    gen->builder.CreateCall( gen->pins("printf")
//                                 , { gen->generateGlobalString("[CM " + type->_name + "] loaded chunk: %p %u <- %zx\n")
//                                   , ch_data
//                                   , ch_len
//                                   , chunkid
//                                   }
//                                 );
    return call;
}

Value* ChunkMapper::generatePut(Value* chunk) {
    if(chunk->getType()->isPointerTy()) {
        chunk = gen->builder.CreateLoad(chunk);
        assert(0 && "this should not work");
    }
    assert(chunk->getType() == gen->t_chunk);
    auto call = gen->builder.CreateCall( gen->pins("pins_chunk_put64")
                                             , {userContext, ConstantInt::get(gen->t_int, idx), chunk}
                                             , "chunkid." + type->_name
                                             );
//    gen->setDebugLocation(call, __FILE__, __LINE__ - 4);
//    Value* ch_data = gen->generateChunkGetData(chunk);
//    Value* ch_len = gen->generateChunkGetLen(chunk);
//    gen->builder.CreateCall( gen->pins("printf")
//            , { gen->generateGlobalString("[CM " + type->_name + "] stored chunk: %p %u -> %zx\n")
//                                           , ch_data
//                                           , ch_len
//                                           , call
//                                   }
//    );
    return call;
}

Value* ChunkMapper::generatePut(Value* len, Value* data) {
    len = gen->builder.CreateIntCast(len, gen->t_chunk->getElementType(0), false);
    data = gen->builder.CreatePointerCast(data, gen->t_chunk->getElementType(1));
    Value* chunk = gen->builder.CreateInsertValue(UndefValue::get(gen->t_chunk), len, {0});
    chunk = gen->builder.CreateInsertValue(chunk, data, {1});
    return generatePut(chunk);
}

Value* ChunkMapper::generatePutAt(Value* len, Value* data, int chunkID) {
    assert(isa<IntegerType>(len->getType()));
    assert(isa<PointerType>(data->getType()));
    len = gen->builder.CreateIntCast(len, gen->t_chunk->getElementType(0), false);
    data = gen->builder.CreatePointerCast(data, gen->t_chunk->getElementType(1));
    Value* chunk = gen->builder.CreateInsertValue(UndefValue::get(gen->t_chunk), len, {0});
    chunk = gen->builder.CreateInsertValue(chunk, data, {1});
    auto vChunkID = ConstantInt::get(gen->t_int, chunkID);
    auto call = gen->builder.CreateCall( gen->pins("pins_chunk_put_at64")
                                             , {userContext, ConstantInt::get(gen->t_int, idx)
                                               , chunk, vChunkID
                                               }
                                             );
    (void)call;
    //    gen->setDebugLocation(call, __FILE__, __LINE__ - 5);
    return vChunkID;
}

Value* ChunkMapper::generateGetAndCopy(Value* chunkid, Value*& ch_len, Value* appendSize) {
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gen->builder.CreateLoad(chunkid);
    }
    if(!chunkid->getType()->isIntegerTy()) {
        assert(0);
    }
    Value* ch = generateGet(chunkid);
    Value* ch_data = gen->generateChunkGetData(ch);
    ch_len = gen->generateChunkGetLen(ch);
    if(appendSize) {
        auto newLength = gen->builder.CreateAdd(appendSize, ch_len);
        auto copy = gen->builder.CreateAlloca(gen->t_int8, newLength);
//        gen->setDebugLocation(copy, __FILE__, __LINE__ - 1);
        auto memcopy = gen->builder.CreateMemCpy( copy
                                                      , copy->getPointerAlignment(gen->dmcModule->getDataLayout())
                                                      , ch_data
                                                      , ch_data->getPointerAlignment(gen->dmcModule->getDataLayout())
                                                      , ch_len
                                                      );
//        gen->setDebugLocation(memcopy, __FILE__, __LINE__ - 6);

        auto memst = gen->builder.CreateMemSet( gen->builder.CreateGEP(copy, {ch_len})
                                                     , ConstantInt::get(gen->t_int8, 0)
                                                     , appendSize
                                                     , MaybeAlign(1)
                                                     );
//        gen->setDebugLocation(memst, __FILE__, __LINE__ - 5);
//        gen->builder.CreateCall( gen->pins("printf")
//                , { gen->generateGlobalString("[CM " + type->_name + "] copied chunk and extended to: %p %u\n")
//                                               , copy
//                                               , newLength
//                                       }
//        );
        (void)memcopy;
        (void)memst;
        return copy;
    } else {
        auto copy = gen->builder.CreateAlloca(gen->t_int8, ch_len);
//        gen->setDebugLocation(copy, __FILE__, __LINE__ - 1);
        auto memcopy = gen->builder.CreateMemCpy( copy
                                                      , copy->getPointerAlignment(gen->dmcModule->getDataLayout())
                                                      , ch_data
                                                      , ch_data->getPointerAlignment(gen->dmcModule->getDataLayout())
                                                      , ch_len
                                                      );
//        gen->setDebugLocation(memcopy, __FILE__, __LINE__ - 6);
//        gen->builder.CreateCall( gen->pins("printf")
//                , { gen->generateGlobalString("[CM " + type->_name + "] copied chunk to: %p %u <- %zx\n")
//                                               , copy
//                                               , ch_len
//                                               , chunkid
//                                       }
//        );
        (void)memcopy;
        return copy;
    }
}

Value* ChunkMapper::generateCloneAndModify(Value* chunkid, Value* offset, Value* data, Value* len) {
    //assert(0 && "unknown if the CAM version works");
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gen->builder.CreateLoad(chunkid, "chunkid." + type->_name + ".before");
    }
//    newLength = gen->builder.CreateAdd(gen->builder.CreateIntCast(offset, gen->t_int, false), gen->builder.CreateIntCast(len, gen->t_int, false));
//    len = gen->builder.CreateIntCast(len, newLength->getType(), false);
//    auto cmp = gen->builder.CreateICmpUGE(newLength, len);
//    newLength = gen->builder.CreateSelect(cmp, newLength, len);
    auto f = gen->pins("pins_chunk_cam64");
    auto call = gen->builder.CreateCall( f
                                             , { userContext
                                               , ConstantInt::get(gen->t_int, idx)
                                               , gen->builder.CreateIntCast(chunkid, f->getFunctionType()->getParamType(2), false)
                                               , gen->builder.CreateIntCast(offset, f->getFunctionType()->getParamType(3), false)
                                               , gen->builder.CreatePointerCast(data, f->getFunctionType()->getParamType(4))
                                               , gen->builder.CreateIntCast(len, f->getFunctionType()->getParamType(5), false)
                                               }
                                             , "chunkid." + type->_name + ".after"
                                             );
//    gen->setDebugLocation(call, __FILE__, __LINE__ - 10);
    return call;
}

Value* ChunkMapper::generatePartialGet(Value* chunkid, Value* offset, Value* data, Value* len) {
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gen->builder.CreateLoad(chunkid);
    }
    auto f = gen->pins("pins_chunk_getpartial64");
    auto call = gen->builder.CreateCall( f
                                             , { userContext
                                               , ConstantInt::get(gen->t_int, idx)
                                               , gen->builder.CreateIntCast(chunkid, f->getFunctionType()->getParamType(2), false)
                                               , gen->builder.CreateIntCast(offset, f->getFunctionType()->getParamType(3), false)
                                               , gen->builder.CreatePointerCast(data, f->getFunctionType()->getParamType(4))
                                               , gen->builder.CreateIntCast(len, f->getFunctionType()->getParamType(5), false)
                                               }
                                             , "chunkid." + type->_name
    );
//    gen->setDebugLocation(call, __FILE__, __LINE__ - 10);
    return call;
}

Value* ChunkMapper::generateCloneAndAppend(Value* chunkid, Value* data, Value* len, Value** oldLength) {
    //assert(0 && "unknown if the CAM version works");
    if(chunkid->getType()->isPointerTy()) {
        chunkid = gen->builder.CreateLoad(chunkid, "chunkid." + type->_name + ".before.append");
    }
//    newLength = gen->builder.CreateAdd(gen->builder.CreateIntCast(offset, gen->t_int, false), gen->builder.CreateIntCast(len, gen->t_int, false));
//    len = gen->builder.CreateIntCast(len, newLength->getType(), false);
//    auto cmp = gen->builder.CreateICmpUGE(newLength, len);
//    newLength = gen->builder.CreateSelect(cmp, newLength, len);
    auto f = gen->pins("pins_chunk_append64");
    auto call = gen->builder.CreateCall( f
            , { userContext
                                                       , ConstantInt::get(gen->t_int, idx)
                                                       , gen->builder.CreateIntCast(chunkid, f->getFunctionType()->getParamType(2), false)
                                                       , gen->builder.CreatePointerCast(data, f->getFunctionType()->getParamType(3))
                                                       , gen->builder.CreateIntCast(len, f->getFunctionType()->getParamType(4), false)
                                               }
            , "chunkid." + type->_name + ".after.append"
    );
//    gen->setDebugLocation(call, __FILE__, __LINE__ - 10);
    return call;
//    Value* ch = generateGet(chunkid);
//    Value* chData = gen->generateChunkGetData(ch);
//    Value* chLen = gen->generateChunkGetLen(ch);
//    if(oldLength) *oldLength = chLen;
//    Value* newLength = gen->builder.CreateAdd(chLen,len);
//    auto copy = gen->builder.CreateAlloca(gen->t_int8, newLength);
//    gen->builder.CreateMemCpy( copy
//                                   , 1
//                                   , chData
//                                   , 1
//                                   , chLen
//                                   );
//    if(data) {
//        gen->builder.CreateMemCpy( gen->builder.CreateGEP(copy, {chLen})
//                                       , MaybeAlign(1)
//                                       , data
//                                       , MaybeAlign(1)
//                                       , len
//                                       );
//    } else {
//        gen->builder.CreateMemSet( gen->builder.CreateGEP(copy, {chLen})
//                                       , ConstantInt::get(gen->t_int8, 0)
//                                       , len
//                                       , MaybeAlign(1)
//                                       );
//    }
//    return generatePut(newLength, copy);
}

Value* ChunkMapper::generateCloneAndAppend(Value* chunkid, Value* data) {
    Value* ch = generateGet(chunkid);
    Value* chData = gen->generateChunkGetData(ch);
    Value* chLen = gen->generateChunkGetLen(ch);

    Value* len = gen->generateAlignedSizeOf(data);
    Value* newLength = gen->builder.CreateAdd(chLen,len);
    auto copy = gen->builder.CreateAlloca(gen->t_int8, newLength);
    gen->builder.CreateMemCpy( copy
                                   , copy->getPointerAlignment(gen->dmcModule->getDataLayout())
                                   , chData
                                   , chData->getPointerAlignment(gen->dmcModule->getDataLayout())
                                   , chLen
                                   );
    auto target = gen->builder.CreateGEP(copy, {chLen});
    target = gen->builder.CreatePointerCast(target, data->getType()->getPointerTo());
    gen->builder.CreateStore(data, target);
    return generatePut(newLength, copy);
}

} // namespace llmc
