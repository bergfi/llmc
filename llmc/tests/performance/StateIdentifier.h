#pragma once

template<typename StateSlot>
struct StateIdentifier {
    using MemOffset = uint64_t;
    size_t getLength() const {
        return data >> 40;
    }

    template<typename Context>
    void pull(Context* ctx, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, 0, slots, getLength(), isRoot);
    }

    template<typename Context>
    void pull(Context* ctx, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, 0, (StateSlot*)slots, getLength(), isRoot);
    }

    template<typename Context>
    void pullPartial(Context* ctx, size_t offset, size_t length, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset, slots, length, isRoot);
    }

    template<typename Context>
    void pullPartial(Context* ctx, size_t offset, size_t length, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset, (StateSlot*)slots, length, isRoot);
    }

    template<typename Context>
    void push(Context* ctx, StateSlot* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newTransition(ctx, length, slots, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, length, slots).getData();
        }
    }

    template<typename Context>
    void push(Context* ctx, StateSlot* slots, size_t length, std::string const& label) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();

        StateIdentifier labelID = {0};
        if(label.length() >= 8) {
            labelID.init(ctx, (StateSlot*) label.c_str(), label.length() / 4);
        }

        data = mc->newTransition(ctx, length, slots, TransitionInfoUnExpanded::construct(labelID)).getData();
    }

    template<typename Context>
    void push(Context* ctx, void* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newTransition(ctx, length, (StateSlot*)slots, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, length, (StateSlot*)slots).getData();
        }
    }

    template<typename Context>
    void init(Context* ctx, StateSlot* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newState(ctx, 0, length, slots).getState().getData();
        } else {
            data = mc->newSubState(ctx, length, slots).getData();
        }
    }

    template<typename Context>
    void initBytes(Context* ctx, StateSlot* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newState(ctx, 0, length / sizeof(StateSlot), slots).getState().getData();
        } else {
            data = mc->newSubState(ctx, length / sizeof(StateSlot), slots).getData();
        }
    }

    template<typename Context>
    void init(Context* ctx, void* slots, size_t length, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        if(isRoot) {
            data = mc->newState(ctx, 0, length, (StateSlot*)slots).getState().getData();
        } else {
            data = mc->newSubState(ctx, length, (StateSlot*)slots).getData();
        }
    }

    template<typename Context>
    size_t appendBytes(Context* ctx, size_t length, const void* slots, bool isRoot = false) {
        size_t len = getLength();
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        auto d = mc->newDelta(len, (StateSlot*)slots, length/4);
        if(isRoot) {
            data = mc->newTransition(ctx, *d, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, data, *d).getData();
        }
        mc->deleteDelta(d);
        return len * 4;
    }

    template<typename Context>
    void modifyBytes(Context* ctx, size_t offset, size_t length, const void* slots, bool isRoot = false) {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        auto d = mc->newDelta(offset/4, (StateSlot*)slots, length/4);
        if(isRoot) {
            data = mc->newTransition(ctx, *d, TransitionInfoUnExpanded::None()).getData();
        } else {
            data = mc->newSubState(ctx, data, *d).getData();
        }
        mc->deleteDelta(d);
    }

    template<typename Context>
    void readBytes(Context* ctx, size_t offset, size_t length, StateSlot* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset/4, slots, length/4, isRoot);
    }

    template<typename Context>
    void readBytes(Context* ctx, size_t offset, size_t length, void* slots, bool isRoot = false) const {
        VModelChecker<llmc::storage::StorageInterface>* mc = ctx->getModelChecker();
        mc->getStatePartial(ctx, data, offset/4, (StateSlot*)slots, length/4, isRoot);
    }

    template<typename Context, typename T>
    void readMemory(Context* ctx, size_t offset, T& var, bool isRoot = false) const {
        readBytes(ctx, offset, sizeof(T), &var, isRoot);
    }

    template<typename Context, typename T>
    void writeMemory(Context* ctx, size_t offset, T const& var, bool isRoot = false) {
        modifyBytes(ctx, offset, sizeof(T), &var, isRoot);
    }

    template<typename Context, typename T>
    bool cas(Context* ctx, MemOffset ptr, T& expected, T const& desired) {
        T localCopy;
        readBytes(ctx, ptr, sizeof(T), &localCopy);

        // CAS failed
        if(memcmp(&localCopy, &expected, sizeof(T))) {
            memcpy(&expected, &localCopy, sizeof(T));
            return true;

            // CAS succeeded
        } else {
            modifyBytes(ctx, ptr, sizeof(T), &desired);
            return false;
        }
    }

    uint64_t data;
};