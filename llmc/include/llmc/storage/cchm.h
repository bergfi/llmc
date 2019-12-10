#pragma once

#include "interface.h"
#include "../../../../../httest/chaintableUBVKmem.h"

namespace hashtables {

template<>
struct key_accessor<llmc::storage::StorageInterface::FullState> {

    __attribute__((always_inline))
    static const char* data(llmc::storage::StorageInterface::FullState const& key) {
//        printf("data %p\n", key.getData());
        return (const char*)key.getData();
    }

    __attribute__((always_inline))
    static size_t size(llmc::storage::StorageInterface::FullState const& key) {
//        printf("length %u\n", key.getLengthInBytes());
        return key.getLengthInBytes();
    }
};

}

namespace llmc::storage {

class cchm: public StorageInterface {
public:

    cchm(): _store(24) {

    }

    ~cchm() {
    }

    void init() {
    }

    void thread_init() {
        _store.thread_init();
    }

    using StateSlot = StorageInterface::StateSlot;
    using StateID = StorageInterface::StateID;
    using StateTypeID = StorageInterface::StateTypeID;
    using Delta = StorageInterface::Delta;
    using MultiDelta = StorageInterface::MultiDelta;

    StateID find(FullState* state) {
        chaintablegenericUBVKmem::HashTable<FullState>::HTE* result;
        if(_store.get(*state, result)) {
            return (uintptr_t)result;
        }
        return StateID::NotFound();
    }

    StateID find(StateSlot* state, size_t length, bool isRoot) {
        auto fsd = FullStateData<StateSlot>::create(isRoot, length, state);
        auto r = find(fsd);
        FullStateData<StateSlot>::destroy(fsd);
        return r;
    }

    //    InsertedState insertOverwrite(FullState* state) {
//        auto id = _nextID;
//        auto r = _store.insert_or_assign(state, id);
//        return {r.first->second, r.second};
//    }
//    InsertedState insertOverwrite(StateSlot* state, size_t length, bool isRoot) {
//        return insert(FullStateData<StateSlot>::create(isRoot, length, state));
//    }

    InsertedState insert(FullState* state) {
        chaintablegenericUBVKmem::HashTable<FullState>::HTE* id = nullptr;
        bool seen = _store.insert(*state, id);
//        fprintf(stderr, "seen: %u\n", seen);
        return InsertedState((uintptr_t)id, seen);
    }

    InsertedState insert(StateSlot* state, size_t length, bool isRoot) {
        return insert(FullStateData<StateSlot>::create(isRoot, length, state));
    }

    InsertedState insert(StateID const& stateID, Delta const& delta, bool isRoot) {
        FullState* old = get(stateID, isRoot);
        LLMC_DEBUG_ASSERT(old);
        LLMC_DEBUG_ASSERT(old->getLength());

//        fprintf(stderr, "old:");
//        for(int i=0; i < old->getLength(); ++i) {
//            fprintf(stderr, " %x", old->getData()[i]);
//        }
//        fprintf(stderr, "\n");

        size_t newLength = std::max(old->getLength(), delta.getLength() + delta.getOffset());

        StateSlot buffer[newLength];

        memcpy(buffer, old->getData(), old->getLength() * sizeof(StateSlot));
        memcpy(buffer+delta.getOffset(), delta.getData(), delta.getLength() * sizeof(StateSlot));
//        fprintf(stderr, "new:");
//        for(int i=0; i < newLength; ++i) {
//            fprintf(stderr, " %x", buffer[i]);
//        }
//        fprintf(stderr, "\n");

        return insert(buffer, newLength, isRoot);

    }

    // need to do:
    // - external FullState
    // - FullState needs to be tailored for treedbs






    FullState* get(StateID id, bool isRoot) {
        auto hte = (chaintablegenericUBVKmem::HashTable<FullState>::HTE*)id.getData();
//        FullState* state = (FullState*)&hte->_length;
        auto state = FullState::createExternal(isRoot, hte->_length / sizeof(StateSlot), (StateSlot*)hte->_keyData);
        if(state->isRoot() == isRoot) {
            return state;
        } else {
            return nullptr;
        }
    }

    void printStats() {
    }

    static bool constexpr accessToStates() {
        return true;
    }

    static bool constexpr stateHasFixedLength() {
        return false;
    }

    static bool constexpr stateHasLengthInfo() {
        return false;
    }

    static bool constexpr needsThreadInit() {
        return true;
    }

    size_t determineLength(StateID const& s) const {
        return 0;
    }

    size_t getMaxStateLength() const {
        return 1ULL << 32;
    }

private:
    chaintablegenericUBVKmem::HashTable<FullState> _store;
};

} // namespace llmc::storage
