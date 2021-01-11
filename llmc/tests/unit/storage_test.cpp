#include <type_traits>

#include <llmc/storage/stdmap.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>
#include <gtest/gtest.h>
#include <gtest/gtest-typed-test.h>
#include <llmc/storage/cchm.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>

template<typename T>
struct HashCompareMurmur {
    static constexpr uint64_t hashForZero = 0x7208f7fa198a2d81ULL;
    static constexpr uint64_t seedForZero = 0xc6a4a7935bd1e995ULL*8ull;

    __attribute__((always_inline))
    bool equal( const T& j, const T& k ) const {
        return j == k;
    }

    __attribute__((always_inline))
    size_t hash( const T& k ) const {
        return MurmurHash64(&k, sizeof(T), seedForZero);
    }
};

template <typename T>
class StorageTest: public ::testing::Test {
public:
    T value_;
};

using MyTypes = ::testing::Types< /*llmc::storage::TreeDBSStorage<llmc::storage::StdMap>
                                , llmc::storage::TreeDBSStorageModified
                                ,*/ llmc::storage::StdMap
                                , llmc::storage::cchm
                                , llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>
                                >;
//using MyTypes = ::testing::Types<DTreeStorage<SeparateRootSingleHashSet<HashSet<RehasherExit>>>>;
TYPED_TEST_CASE(StorageTest, MyTypes);

TYPED_TEST(StorageTest, BasicInsertLength2) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {65, 66, 67, 68, 69, 70};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[6];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(states)/sizeof(*states));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[1] = storage.insert(&states[1], 2, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    stateIDs[2] = storage.insert(&states[2], 2, true);
    EXPECT_TRUE(stateIDs[2].isInserted());

    stateIDs[3] = storage.insert(&states[3], 2, false);
    EXPECT_TRUE(stateIDs[3].isInserted());

    stateIDs[2] = storage.insert(&states[2], 2, true);
    EXPECT_FALSE(stateIDs[2].isInserted());

    stateIDsFound[1] = storage.find(&states[1], 2, true);
    EXPECT_TRUE(stateIDsFound[1].exists());

    stateIDsFound[2] = storage.find(&states[2], 2, true);
    EXPECT_TRUE(stateIDsFound[2].exists());

    stateIDsFound[3] = storage.find(&states[3], 2, false);
    EXPECT_TRUE(stateIDsFound[3].exists());

    stateIDsFound[4] = storage.find(&states[4], 2, true);
    EXPECT_FALSE(stateIDsFound[4].exists());

    stateIDsFound[5] = storage.find(&states[3], 2, true);
    EXPECT_FALSE(stateIDsFound[5].exists());

    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
}

TYPED_TEST(StorageTest, BasicInsertLength4) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {272, 273, 274, 275, 276, 277, 278, 279};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[6];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(states)/sizeof(*states));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[1] = storage.insert(&states[1], 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    stateIDs[2] = storage.insert(&states[2], 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());

    stateIDs[3] = storage.insert(&states[3], 4, false);
    EXPECT_TRUE(stateIDs[3].isInserted());

    stateIDs[1] = storage.insert(&states[1], 4, true);
    EXPECT_FALSE(stateIDs[1].isInserted());

    stateIDsFound[1] = storage.find(&states[1], 4, true);
    EXPECT_TRUE(stateIDsFound[1].exists());

    stateIDsFound[2] = storage.find(&states[2], 4, true);
    EXPECT_TRUE(stateIDsFound[2].exists());

    stateIDsFound[3] = storage.find(&states[3], 4, false);
    EXPECT_TRUE(stateIDsFound[3].exists());

    stateIDsFound[4] = storage.find(&states[4], 4, true);
    EXPECT_FALSE(stateIDsFound[4].exists());

    stateIDsFound[5] = storage.find(&states[3], 4, true);

    EXPECT_FALSE(stateIDsFound[5].exists());

    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
}

TYPED_TEST(StorageTest, BasicInsertZero) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDsFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(states)/sizeof(*states));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(&states[0], 8, true);
    EXPECT_TRUE(stateID.isInserted());

    stateIDsFound = storage.find(&states[0], 8, true);
    EXPECT_TRUE(stateIDsFound.exists());

    stateID = storage.insert(&states[0], 8, true);
    EXPECT_FALSE(stateID.isInserted());

    stateIDsFound = storage.find(&states[0], 8, true);
    EXPECT_TRUE(stateIDsFound.exists());

    stateID = storage.insert(&states[0], 4, true);
    EXPECT_TRUE(stateID.isInserted());

    stateID = storage.insert(&states[0], 16, true);
    EXPECT_TRUE(stateID.isInserted());

    stateID = storage.insert(&states[0], 2, true);
    EXPECT_TRUE(stateID.isInserted());

    stateIDsFound = storage.find(&states[0], 10, true);
    EXPECT_FALSE(stateIDsFound.exists());
}

TYPED_TEST(StorageTest, BasicInsertZero32) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // 0 0 -> a
    // a a -> b
    // b b -> c
    // c c -> d
    // d d -> e

    // 1 0 -> a
    // 0 0 -> b
    // a b -> c
    // b b -> d
    // c d -> e
    // d d -> f
    // e f -> g
    // f f -> h
    // g h -> i

    // not store 0 0

    // nothing (maybe root node)

    // 0 0  is stored in treedbs
    // 1 0 -> a
    // a b -> c
    // c d -> e
    // e f -> g
    // g 0 -> i

    // dtree:
    // 1 0 -> a
    // a 0 -> b

    // b 0 -> c (root so 16 bytes)

    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDsFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(states)/sizeof(*states));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(states, sizeof(states)/sizeof(*states), true);
    EXPECT_TRUE(stateID.isInserted());

    auto stats = storage.getStatistics();
    printf("_bytesInUse: %zu\n", stats._bytesInUse);

}

TYPED_TEST(StorageTest, BasicDuplicateInsert) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {0, 1, 2, 3, 4, 5};
    typename TypeParam::InsertedState stateIDs[6];
    typename TypeParam::StateID stateIDsFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(4);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    // First insert
    stateIDs[1] = storage.insert(&states[0], 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    // Duplicate insert
    stateIDs[2] = storage.insert(&states[0], 4, true);
    EXPECT_FALSE(stateIDs[2].isInserted());
    EXPECT_EQ(stateIDs[2].getState(), stateIDs[1].getState());

    // New insert
    stateIDs[3] = storage.insert(&states[2], 4, true);
    EXPECT_TRUE(stateIDs[3].isInserted());
    EXPECT_NE(stateIDs[3].getState(), stateIDs[1].getState());

    // Duplicate data but non-root insert
    stateIDs[4] = storage.insert(&states[2], 4, false);
    EXPECT_TRUE(stateIDs[4].isInserted());

    // Find the duplicate
    stateIDsFound = storage.find(&states[0], 4, true);
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound);
}

TYPED_TEST(StorageTest, BasicDeltaTransition) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::InsertedState stateIDs[2];
    typename TypeParam::StateID stateIDsFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[0] = storage.insert(state1, 4, true);
    stateIDsFound = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDs[0].isInserted());
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[0].getState(), stateIDsFound);

    typename TypeParam::Delta* delta = TypeParam::Delta::create(0, state2, 4);
    stateIDs[1] = storage.insert(stateIDs[0].getState(), *delta, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    stateIDsFound = storage.find(state2, 4, true);
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound);

    if(stateIDs[1].getState() != stateIDsFound && stateIDsFound.getData() > 0 && stateIDs[1].getState().getData() > 0) {
        auto fsd1 = storage.get(stateIDs[1].getState(), true);
        auto fsd2 = storage.get(stateIDsFound, true);
        auto stateBuffer = fsd1->getData();
        fprintf(stderr, "stateIDs[1]: %x %x %x %x\n", stateBuffer[0], stateBuffer[1], stateBuffer[2], stateBuffer[3]);
        stateBuffer = fsd2->getData();
        fprintf(stderr, "stateIDsFound: %x %x %x %x\n", stateBuffer[0], stateBuffer[1], stateBuffer[2], stateBuffer[3]);
    }
    TypeParam::Delta::destroy(delta);
}

TYPED_TEST(StorageTest, BasicLoopTransition) {
    TypeParam storage;

    typename TypeParam::StateSlot states[] = {0, 1, 2, 3};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(states)/sizeof(*states));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[1] = storage.insert(&states[1], 2, true); // insert {1,2}
    stateIDsFound[1] = storage.find(&states[1], 2, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);

    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(0, &states[2], 2);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true); // insert {1,2} -> {2,3}
    stateIDsFound[2] = storage.find(&states[2], 2, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    TypeParam::Delta::destroy(delta1);

    typename TypeParam::Delta* delta2 = TypeParam::Delta::create(0, &states[1], 2);
    stateIDs[3] = storage.insert(stateIDs[2].getState(), *delta2, true); // insert {2,3} -> {1,2}
    stateIDsFound[3] = storage.find(&states[1], 2, true);
    EXPECT_FALSE(stateIDs[3].isInserted());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDs[1].getState());
    TypeParam::Delta::destroy(delta2);

    if(stateIDs[3].getState() != stateIDs[1].getState()) {
        auto fsd1 = storage.get(stateIDs[3].getState(), true);
        auto fsd2 = storage.get(stateIDs[1].getState(), true);
        auto stateBuffer = fsd1->getData();
        fprintf(stderr, "stateIDs[3]: %x %x %x %x\n", stateBuffer[0], stateBuffer[1], stateBuffer[2], stateBuffer[3]);
        stateBuffer = fsd2->getData();
        fprintf(stderr, "stateIDs[1]: %x %x %x %x\n", stateBuffer[0], stateBuffer[1], stateBuffer[2], stateBuffer[3]);
    }
}

TYPED_TEST(StorageTest, MultiSlotGet) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, 4, true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::FullState *fsd = storage.get(stateID.getState(), true);
    EXPECT_EQ(sizeof(state1)/sizeof(*state1), fsd->getLength());
    EXPECT_EQ(sizeof(state1), fsd->getLengthInBytes());
    EXPECT_FALSE(memcmp(state1, fsd->getData(), sizeof(state1)));
}

TYPED_TEST(StorageTest, MultiSlotLoopTransition) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::StateSlot state3[] = {0, 1, 2, 3};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[1] = storage.insert(state1, 4, true);
    stateIDsFound[1] = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);

    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(0, state2, 4);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
    stateIDsFound[2] = storage.find(state2, 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);

    typename TypeParam::Delta* delta2 = TypeParam::Delta::create(0, state3, 4);
    stateIDs[3] = storage.insert(stateIDs[2].getState(), *delta2, true);
    stateIDsFound[3] = storage.find(state3, 4, true);
    EXPECT_FALSE(stateIDs[3].isInserted());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDs[1].getState());
}

TYPED_TEST(StorageTest, MultiSlotLoopTransitionWithOffset) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::StateSlot state3[] = {0, 1, 2, 3};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateIDs[1] = storage.insert(state1, 4, true);
    stateIDsFound[1] = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);

    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(2, state2+2, 2);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
    stateIDsFound[2] = storage.find(state2, 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);

    typename TypeParam::Delta* delta2 = TypeParam::Delta::create(2, state3+2, 2);
    stateIDs[3] = storage.insert(stateIDs[2].getState(), *delta2, true);
    stateIDsFound[3] = storage.find(state3, 4, true);
    EXPECT_FALSE(stateIDs[3].isInserted());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDs[1].getState());
}

TYPED_TEST(StorageTest, MultiSlotDiamondTransition) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    typename TypeParam::StateSlot state2[] = {0x11111111, 0x22222222, 0x33333333, 0x99999999};
    typename TypeParam::StateSlot state3[] = {0x88888888, 0x22222222, 0x33333333, 0x44444444};
    typename TypeParam::StateSlot state4[] = {0x88888888, 0x22222222, 0x33333333, 0x99999999};

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    /*         ---> State 3 ----\
     *        /                  v
     * State 1 ----------------> State 4
     *        \                  ^
     *         ---> State 3 ----/
     */

    typename TypeParam::InsertedState stateIDs[5];
    typename TypeParam::StateID stateIDFound;

    // State 1
    stateIDs[1] = storage.insert(state1, 4, true);
    stateIDFound = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDFound);

    // State 1 ---> State 2
    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(2, state2+2, 2);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
    stateIDFound = storage.find(state2, 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDFound);

    // State 1 ---> State 3
    typename TypeParam::Delta* delta2 = TypeParam::Delta::create(0, state3, 2);
    stateIDs[3] = storage.insert(stateIDs[1].getState(), *delta2, true);
    stateIDFound = storage.find(state3, 4, true);
    EXPECT_TRUE(stateIDs[3].isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[3].getState(), stateIDFound);

    // State 3 ---> State 4
    typename TypeParam::Delta* delta3 = TypeParam::Delta::create(2, state4+2, 2);
    stateIDs[4] = storage.insert(stateIDs[3].getState(), *delta3, true);
    stateIDFound = storage.find(state4, 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[4].getState(), stateIDFound);

    // State 2 ---> State 4
    typename TypeParam::Delta* delta4 = TypeParam::Delta::create(0, state4, 2);
    auto stateIDs4b = storage.insert(stateIDs[2].getState(), *delta4, true);
    stateIDFound = storage.find(state4, 4, true);
    EXPECT_FALSE(stateIDs4b.isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs4b.getState(), stateIDFound);
    EXPECT_EQ(stateIDs4b.getState(), stateIDs[4].getState());

    // State 1 ---> State 4
    typename TypeParam::Delta* delta5 = TypeParam::Delta::create(0, state4, 4);
    auto stateIDs4c = storage.insert(stateIDs[4].getState(), *delta5, true);
    stateIDFound = storage.find(state4, 4, true);
    EXPECT_FALSE(stateIDs4c.isInserted());
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs4c.getState(), stateIDFound);
    EXPECT_EQ(stateIDs4c.getState(), stateIDs[4].getState());
}

TYPED_TEST(StorageTest, Insert2to1024root) {
    TypeParam storage;

    const size_t maxLength = 1024;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == 'a') c = 'A';
        state[i] = c;
    }

    for(size_t length = 2; length <= maxLength; length++) {
        auto stateID = storage.insert(state, length, true);
        EXPECT_TRUE(stateID.isInserted());
        auto stateIDFound = storage.find(state, length, true);
        EXPECT_TRUE(stateIDFound.exists());

        auto fsd = storage.get(stateID.getState(), true);
        assert(fsd);
        EXPECT_FALSE(memcmp(state, fsd->getData(), length * sizeof(typename TypeParam::StateSlot)));

        memset(state2, 0, length * sizeof(typename TypeParam::StateSlot));
        bool exists = storage.get(state2, stateID.getState(), true);
        EXPECT_TRUE(exists);
        EXPECT_FALSE(memcmp(state, state2, length * sizeof(typename TypeParam::StateSlot)));
    }
}

TYPED_TEST(StorageTest, Insert2to1024nonroot) {
    TypeParam storage;

    const size_t maxLength = 1024;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == 'a') c = 'A';
        state[i] = c;
    }

    for(size_t length = 2; length <= maxLength; length++) {
        auto stateID = storage.insert(state, length, false);
        auto stateIDFound = storage.find(state, length, false);
        EXPECT_TRUE(stateIDFound.exists());

        auto fsd = storage.get(stateID.getState(), false);
        assert(fsd);
        EXPECT_FALSE(memcmp(state, fsd->getData(), length * sizeof(typename TypeParam::StateSlot)));

        memset(state2, 0, length * sizeof(typename TypeParam::StateSlot));
        bool exists = storage.get(state2, stateID.getState(), false);
        EXPECT_TRUE(exists);
        EXPECT_FALSE(memcmp(state, state2, length * sizeof(typename TypeParam::StateSlot)));

    }
}

TYPED_TEST(StorageTest, PartialGet2to1024nonroot) {
    TypeParam storage;

    const size_t maxLength = 1024;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == 'a') c = 'A';
        state[i] = c;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(size_t posA = 0; posA < maxLength; posA++) {
        for(size_t posB = posA+1; posB < maxLength; posB++) {
            size_t length = posB - posA;
            memset(state2, 0, length * sizeof(typename TypeParam::StateSlot));
            bool exists = storage.getPartial(stateID.getState(), posA, state2, length, false);
            EXPECT_TRUE(exists);
            EXPECT_FALSE(memcmp(state+posA, state2, length * sizeof(typename TypeParam::StateSlot)));
        }
    }
}

TYPED_TEST(StorageTest, PartialGet2to1024root) {
    TypeParam storage;

    const size_t maxLength = 1024;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == 'a') c = 'A';
        state[i] = c;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(size_t posA = 0; posA < maxLength; posA++) {
        for(size_t posB = posA+1; posB < maxLength; posB++) {
            size_t length = posB - posA;
            memset(state2, 0, length * sizeof(typename TypeParam::StateSlot));
            bool exists = storage.getPartial(stateID.getState(), posA, state2, length, true);
            EXPECT_TRUE(exists);
            if(memcmp(state+posA, state2, length * sizeof(typename TypeParam::StateSlot))) {
                printf("  Actual:");
                for(size_t i = 0; i < length; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < length; ++i) {
                    printf(" %02x", state[posA+i]);
                }
                printf("\n");
                EXPECT_TRUE(false);
            }
            return;
        }
    }
}

TYPED_TEST(StorageTest, InsertLongString) {
    TypeParam storage;

    const std::string str = "; r[2]  {main}   %3 = alloca i64, align 8; r[3]  {main}   %4 = alloca i64, align 8; r[4]  {main}   %5 = alloca i64, align 8; r[5]  {main}   %6 = alloca i64, align 8; r[6]  {main}   %7 = bitcast i64* %3 to i8*;  {main}   call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %7) #6; r[7]  {main}   %8 = bitcast i64* %4 to i8*;  {main}   call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %8) #6; r[8]  {main}   %9 = bitcast i64* %5 to i8*;  {main}   call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %9) #6;  {main}   store i64 0, i64* %5, align 8, !tbaa !4; r[9]  {main}   %10 = bitcast i64* %6 to i8*;  {main}   call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %10) #6";

    size_t slots = (str.length() / sizeof(typename TypeParam::StateSlot) * sizeof(char))  &~1ULL;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(slots);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    auto stateID = storage.insert((typename TypeParam::StateSlot*)str.c_str(), slots, false);
    auto stateIDFound = storage.find((typename TypeParam::StateSlot*)str.c_str(), slots, false);
    EXPECT_TRUE(stateIDFound.exists());

    auto fsd = storage.get(stateID.getState(), false);
    assert(fsd);
    EXPECT_FALSE(memcmp(str.c_str(), fsd->getData(), slots * sizeof(typename TypeParam::StateSlot)));

    typename TypeParam::StateSlot readback[slots];

    storage.get(readback, stateID.getState(), false);
    EXPECT_FALSE(memcmp(str.c_str(), readback, slots * sizeof(typename TypeParam::StateSlot)));
}

TYPED_TEST(StorageTest, DeltaAsAppend) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {1, 2, 3, 4};
    typename TypeParam::StateSlot delta[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    typename TypeParam::StateSlot state2correct[] = {1, 2, 3, 4, 0x11111111, 0x22222222, 0x33333333, 0x44444444};

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state2correct)/sizeof(*state2correct));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    typename TypeParam::InsertedState stateIDs[5];
    typename TypeParam::StateID stateIDFound;

    // State 1
    stateIDs[1] = storage.insert(state1, 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    stateIDFound = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDFound);

    // State 1 ---> State 2
    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(4, delta, 4);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
    EXPECT_TRUE(stateIDs[2].isInserted());

    stateIDFound = storage.find(state2correct, 8, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDFound);

}

TYPED_TEST(StorageTest, DeltaOne) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {17, 18, 19, 20, 21, 22, 23, 24};
    size_t nrSlots = sizeof(state1) / sizeof(*state1);

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(nrSlots);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    typename TypeParam::InsertedState stateIDs[5];
    typename TypeParam::StateID stateIDFound;

    for(size_t length = 2; length < nrSlots; length++) {

        // State 1
        stateIDs[1] = storage.insert(state1, length, true);
        EXPECT_TRUE(stateIDs[1].isInserted());
        stateIDFound = storage.find(state1, length, true);
        EXPECT_TRUE(stateIDFound.exists());
        EXPECT_EQ(stateIDs[1].getState(), stateIDFound);

        for(size_t offset = 0; offset < length; ++offset) {
            typename TypeParam::StateSlot delta[] = {9};
            typename TypeParam::StateSlot state2correct[8];

            memcpy(state2correct, state1, sizeof(state1));
            state2correct[offset] = *delta;

            // State 1 ---> State 2
            typename TypeParam::Delta* delta1 = TypeParam::Delta::create(offset, delta, 1);
            stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
            EXPECT_TRUE(stateIDs[2].isInserted());

            stateIDFound = storage.find(state2correct, length, true);
            if(!stateIDFound.exists()) {
                EXPECT_TRUE(false);
            }
            EXPECT_EQ(stateIDs[2].getState(), stateIDFound);
        }
    }

}

TYPED_TEST(StorageTest, DeltaStressTestNonRoot) {
    TypeParam storage;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot delta[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(size_t posA = 0; posA < maxLength; posA++) {
        for(size_t posB = posA+1; posB <= maxLength; posB++) {
            size_t length = posB - posA;

            auto newState = storage.insert(stateID.getState(), posA, length, delta, false);

            memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
            memcpy(stateCorrect + posA, delta, length * sizeof(typename TypeParam::StateSlot));

            bool exists = storage.get(state2, newState.getState(), false);
            EXPECT_TRUE(exists);
            if(memcmp(stateCorrect, state2, maxLength * sizeof(typename TypeParam::StateSlot))) {
                printf("  Actual:");
                for(size_t i = 0; i < maxLength; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < maxLength; ++i) {
                    printf(" %02x", stateCorrect[i]);
                }
                printf("\n");
            }
        }
    }
}

TYPED_TEST(StorageTest, DeltaStressTestRoot) {
    TypeParam storage;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot delta[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateID.isInserted());
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(size_t posA = 0; posA < maxLength; posA++) {
        for(size_t posB = posA+1; posB <= maxLength; posB++) {
            size_t length = posB - posA;

//            printf("------------------------\n");
            auto newState = storage.insert(stateID.getState(), posA, length, delta, true);
            if(!newState.isInserted()) {
                printf("Not inserted, using delta %zu@%zu\n", length, posA);
                EXPECT_TRUE(false);
            }

            memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
            memcpy(stateCorrect + posA, delta, length * sizeof(typename TypeParam::StateSlot));

            bool exists = storage.get(state2, newState.getState(), true);
            EXPECT_TRUE(exists);
            if(memcmp(stateCorrect, state2, maxLength * sizeof(typename TypeParam::StateSlot))) {
                printf("  Actual:");
                for(size_t i = 0; i < maxLength; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < maxLength; ++i) {
                    printf(" %02x", stateCorrect[i]);
                }
                printf("\n");
                FAIL();
            }
        }
    }
}

TYPED_TEST(StorageTest, ExtendStressTestRoot) {
    TypeParam storage;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot delta[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    for(size_t posA = 2; posA < maxLength; posA++) {
        size_t lengthOriginal = posA;
        auto stateID = storage.insert(state, lengthOriginal, true);
        EXPECT_TRUE(stateID.isInserted());
        auto stateIDFound = storage.find(state, lengthOriginal, true);
        EXPECT_TRUE(stateIDFound.exists());

        for(size_t posB = posA+1; posB <= maxLength; posB++) {
            size_t lengthDelta = posB - posA;

            auto newState = storage.insert(stateID.getState(), lengthOriginal, lengthDelta, delta, true);
            if(!newState.isInserted()) {
                printf("Not inserted, using delta %zu@%zu\n", lengthDelta, lengthOriginal);
                EXPECT_TRUE(false);
            }

            memcpy(stateCorrect, state, lengthOriginal * sizeof(typename TypeParam::StateSlot));
            memcpy(stateCorrect + lengthOriginal, delta, lengthDelta * sizeof(typename TypeParam::StateSlot));

            bool exists = storage.get(state2, newState.getState(), true);
            EXPECT_TRUE(exists);
            if(memcmp(stateCorrect, state2, (lengthOriginal + lengthDelta) * sizeof(typename TypeParam::StateSlot))) {
                printf("== Error: lengthOriginal=%zu, lengthDelta=%zu\n", lengthOriginal, lengthDelta);
                printf("Original:");
                for(size_t i = 0; i < lengthOriginal; ++i) {
                    printf(" %02x", state[i]);
                }
                printf("\n");
                printf("   Delta:");
                for(size_t i = 0; i < lengthDelta; ++i) {
                    printf(" %02x", delta[i]);
                }
                printf("\n");
                printf("  Actual:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", stateCorrect[i]);
                }
                printf("\n");
                EXPECT_TRUE(false);
                exit(1);
            }
        }
    }
}

TYPED_TEST(StorageTest, AppendStressTestRoot) {
    TypeParam storage;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot delta[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    for(size_t posA = 2; posA < maxLength; posA++) {
        size_t lengthOriginal = posA;
        auto stateID = storage.insert(state, lengthOriginal, true);
        EXPECT_TRUE(stateID.isInserted());
        auto stateIDFound = storage.find(state, lengthOriginal, true);
        EXPECT_TRUE(stateIDFound.exists());

        for(size_t posB = posA+1; posB <= maxLength; posB++) {
            size_t lengthDelta = posB - posA;

            auto newState = storage.append(stateID.getState(), lengthDelta, delta, true);
            if(!newState.isInserted()) {
                printf("Not inserted, using delta %zu@%zu\n", lengthDelta, lengthOriginal);
                EXPECT_TRUE(false);
            }

            memcpy(stateCorrect, state, lengthOriginal * sizeof(typename TypeParam::StateSlot));
            memcpy(stateCorrect + lengthOriginal, delta, lengthDelta * sizeof(typename TypeParam::StateSlot));

            bool exists = storage.get(state2, newState.getState(), true);
            EXPECT_TRUE(exists);
            if(memcmp(stateCorrect, state2, (lengthOriginal + lengthDelta) * sizeof(typename TypeParam::StateSlot))) {
                printf("== Error: lengthOriginal=%zu, lengthDelta=%zu\n", lengthOriginal, lengthDelta);
                printf("Original:");
                for(size_t i = 0; i < lengthOriginal; ++i) {
                    printf(" %02x", state[i]);
                }
                printf("\n");
                printf("   Delta:");
                for(size_t i = 0; i < lengthDelta; ++i) {
                    printf(" %02x", delta[i]);
                }
                printf("\n");
                printf("  Actual:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", stateCorrect[i]);
                }
                printf("\n");
                EXPECT_TRUE(false);
                exit(1);
            }
        }
    }
}

TYPED_TEST(StorageTest, ExtendStressTestNonRoot) {
    TypeParam storage;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot delta[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    for(size_t posA = 1; posA < maxLength; posA++) {
        for(size_t posB = posA+1; posB <= maxLength; posB++) {
            size_t lengthOriginal = posA;
            size_t lengthDelta = posB - posA;

            auto stateID = storage.insert(state, lengthOriginal, false);
            auto stateIDFound = storage.find(state, lengthOriginal, false);
            EXPECT_TRUE(stateIDFound.exists());

            auto newState = storage.insert(stateID.getState(), lengthOriginal, lengthDelta, delta, false);

            memcpy(stateCorrect, state, lengthOriginal * sizeof(typename TypeParam::StateSlot));
            memcpy(stateCorrect + lengthOriginal, delta, lengthDelta * sizeof(typename TypeParam::StateSlot));

            bool exists = storage.get(state2, newState.getState(), false);
            EXPECT_TRUE(exists);
            if(memcmp(stateCorrect, state2, (lengthOriginal + lengthDelta) * sizeof(typename TypeParam::StateSlot))) {
                printf("  Actual:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", state2[i]);
                }
                printf("\n");
                printf("Expected:");
                for(size_t i = 0; i < lengthOriginal + lengthDelta; ++i) {
                    printf(" %02x", stateCorrect[i]);
                }
                printf("\n");
                EXPECT_TRUE(false);
            }
        }
    }
}

TYPED_TEST(StorageTest, MultiProjection_getPartial_singleLevel) {
    printf("skipped\n");
    return;
    TypeParam storage;

//    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 63;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
//        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    uint32_t buffer[TypeParam::MultiProjection::getRequiredBufferSize32B(2,2)];
                    typename TypeParam::MultiProjection& projection = TypeParam::MultiProjection::create(buffer, 2, 2);
                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE, posB-posA, {posA});
                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE, posD-posC, {posC});

                    storage.getPartial(stateID.getState(), projection, true, state2);

                    memcpy(stateCorrect          , state+posA, (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posB-posA, state+posC, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    if(memcmp(stateCorrect, state2, ((posB-posA) + (posD-posC)) * sizeof(typename TypeParam::StateSlot))) {
                        printf("---------- %u %u %u %u\n", posA, posB, posC, posD);
                        printf("  Actual:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                        abort();
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, MultiProjection_delta_singleLevel) {
    printf("skipped\n");
    return;
    TypeParam storage;

//    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    uint32_t buffer[TypeParam::MultiProjection::getRequiredBufferSize32B(2,2)];
                    typename TypeParam::MultiProjection& projection = TypeParam::MultiProjection::create(buffer, 2, 2);
                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE, posB-posA, {posA});
                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE, posD-posC, {posC});

//                    printf("---------- %u %u %u %u\n", posA, posB, posC, posD);

                    auto newStateID = storage.delta(stateID.getState(), projection, false, delta);

                    memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posA, delta          , (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posC, delta+posB-posA, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect)/sizeof(*stateCorrect), false);
                    EXPECT_TRUE(newStateIDFound.exists());

                    bool exists = storage.get(state2, newStateID.getState(), false);
                    EXPECT_TRUE(exists);
                    if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                        printf("  Actual:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                        abort();
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, MultiProjection_delta_twoLevel) {
    printf("skipped\n");
    return;
    TypeParam storage;

//    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 32;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateNew[maxLength];
    typename TypeParam::StateSlot delta[maxLength*5];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == '[') c = 'A';
        state[i] = c;
    }
    for(size_t i = 0; i < sizeof(delta)/sizeof(*delta); ++i, ++d) {
        if(d == ':') d = '0';
        delta[i] = d;
    }

    auto originalStateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto originalStateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(originalStateIDFound.exists());

    for(uint32_t posRecursiveA = 0; posRecursiveA < maxLength; posRecursiveA+=2) {
        for(uint32_t posRecursiveB = posRecursiveA + 2; posRecursiveB < maxLength; posRecursiveB+=2) {

            memcpy(stateNew, state, sizeof(stateNew));
            uint64_t idx = originalStateID.getState().getData();
            stateNew[posRecursiveA] = idx;
            stateNew[posRecursiveA+1] = idx >> 32;
            stateNew[posRecursiveB] = idx;
            stateNew[posRecursiveB+1] = idx >> 32;
            auto stateID = storage.insert(stateNew, sizeof(stateNew)/sizeof(*stateNew), false);

            for(uint32_t posA = posRecursiveB + 2; posA <= maxLength; posA++) {
                for(uint32_t posB = posA; posB <= maxLength; posB++) {
                    for(uint32_t posC = 0; posC <= maxLength; posC+=8) {
                        for(uint32_t posD = posC + 8; posD <= maxLength; posD+=8) {
                            for(uint32_t posE = 0; posE <= maxLength; posE+=8) {
                                for(uint32_t posF = posE + 8; posF <= maxLength; posF+=8) {

                                    uint32_t deltaPositionPosA = 0;
                                    uint32_t deltaPositionPosC = 0;
                                    uint32_t deltaPositionPosE = 0;


                                    uint32_t buffer[TypeParam::MultiProjection::getRequiredBufferSize32B(5, 3)];
                                    typename TypeParam::MultiProjection& projection = TypeParam::MultiProjection::create(
                                            buffer,
                                            5, 3);
//                                    printf("----------");
//                                    printf(" [%u:%u](%u--%u)", posRecursiveA, posRecursiveA+2, posC, posD);
//                                    printf(" [%u:%u](%u--%u)", posRecursiveB, posRecursiveB+2, posE, posF);
//                                    printf(" <--%u:%u-->", posA, posB);
//                                    printf("\n");
                                    uint32_t deltaPosition = 0;
                                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                             posD - posC,
                                                             {posRecursiveA, posC});
                                    deltaPositionPosC = deltaPosition;
                                    deltaPosition += posD - posC;
                                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                             posF - posE,
                                                             {posRecursiveB, posE});
                                    deltaPositionPosE = deltaPosition;
                                    deltaPosition += posF - posE;
                                    if(posA < posB) {
                                        projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                                 posB - posA,
                                                                 {posA});
                                        deltaPositionPosA = deltaPosition;
                                    }

                                    auto newStateID = storage.delta(stateID.getState(), projection, false, delta);

                                    bool exists = storage.get(state2, newStateID.getState(), false);
                                    EXPECT_TRUE(exists);

                                    bool wrong = false;

                                    if(posA < posB) {
                                        wrong = wrong || memcmp(state2+posA, delta+deltaPositionPosA, (posB-posA) * sizeof(*state));
                                    }

                                    if(wrong) {
                                        printf("  Actual:");
                                        for(size_t i = 0; i < maxLength; ++i) {
                                            printf(" %02x", state2[i]);
                                        }
                                        printf("\n");
                                        printf("Expected:");
                                        for(size_t i = 0; i < maxLength; ++i) {
                                            printf(" %02x", stateNew[i]);
                                        }
                                        printf("\n");
                                        EXPECT_TRUE(false);
                                        abort();
                                    }

                                    uint64_t indexA = state2[posRecursiveA+1];
                                    indexA <<= 32;
                                    indexA |= state2[posRecursiveA];
                                    uint64_t indexB = state2[posRecursiveB+1];
                                    indexB <<= 32;
                                    indexB |= state2[posRecursiveB];

                                    {
                                        bool existsA = storage.get(state2, indexA, false);
                                        EXPECT_TRUE(existsA);
                                        typename TypeParam::StateSlot stateCorrect[maxLength];
                                        memcpy(stateCorrect, state, sizeof(state));
                                        memcpy(stateCorrect+posC, delta+deltaPositionPosC, (posD-posC)*sizeof(*state));
                                        if(memcmp(stateCorrect, state2, sizeof(state2))) {
                                            printf("  Actual:");
                                            for(size_t i = 0; i < maxLength; ++i) {
                                                printf(" %02x", state2[i]);
                                            }
                                            printf("\n");
                                            printf("Expected:");
                                            for(size_t i = 0; i < maxLength; ++i) {
                                                printf(" %02x", stateCorrect[i]);
                                            }
                                            printf("\n");
                                            EXPECT_TRUE(false);
                                            abort();
                                        }
                                    }
                                    {
                                        bool existsB = storage.get(state2, indexB, false);
                                        EXPECT_TRUE(existsB);
                                        typename TypeParam::StateSlot stateCorrect[maxLength];
                                        memcpy(stateCorrect, state, sizeof(state));
                                        memcpy(stateCorrect+posE, delta+deltaPositionPosE, (posF-posE)*sizeof(*state));
                                        if(memcmp(stateCorrect, state2, sizeof(state2))) {
                                            printf("  Actual:");
                                            for(size_t i = 0; i < maxLength; ++i) {
                                                printf(" %02x", state2[i]);
                                            }
                                            printf("\n");
                                            printf("Expected:");
                                            for(size_t i = 0; i < maxLength; ++i) {
                                                printf(" %02x", stateCorrect[i]);
                                            }
                                            printf("\n");
                                            EXPECT_TRUE(false);
                                            abort();
                                        }
                                    }

                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, MultiProjection_delta_twoLevel3root) {
    printf("skipped\n");
    return;
    TypeParam storage;

//    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 32;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateNew[maxLength];
    typename TypeParam::StateSlot delta[maxLength*5];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c) {
        if(c == '[') c = 'A';
        state[i] = c;
    }
    for(size_t i = 0; i < sizeof(delta)/sizeof(*delta); ++i, ++d) {
        if(d == ':') d = '0';
        delta[i] = d;
    }

    auto originalStateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto originalStateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(originalStateIDFound.exists());

    constexpr bool printThings = false;

    for(uint32_t posRecursiveA = 0; posRecursiveA < maxLength; posRecursiveA+=2) {
        for(uint32_t posRecursiveB = posRecursiveA + 2; posRecursiveB < maxLength; posRecursiveB+=2) {

            memcpy(stateNew, state, sizeof(stateNew));
            uint64_t idx = originalStateID.getState().getData();
            stateNew[posRecursiveA] = idx;
            stateNew[posRecursiveA+1] = idx >> 32;
            stateNew[posRecursiveB] = idx;
            stateNew[posRecursiveB+1] = idx >> 32;
            auto stateID = storage.insert(stateNew, sizeof(stateNew)/sizeof(*stateNew), false);

            for(uint32_t posA = 0; posA <= posRecursiveA; posA++) {
                for(uint32_t posB = posA; posB <= posRecursiveA; posB++) {
                    for(uint32_t posC = posRecursiveA+2; posC <= posRecursiveB; posC++) {
                        for(uint32_t posD = posC; posD <= posRecursiveB; posD++) {
                            for(uint32_t posE = posRecursiveB+2; posE <= maxLength; posE++) {
                                for(uint32_t posF = posE; posF <= maxLength; posF++) {

//                                    uint32_t deltaPositionPosA = 0;
//                                    uint32_t deltaPositionPosC = 0;
//                                    uint32_t deltaPositionPosE = 0;

                                    typename TypeParam::StateSlot stateCorrect[maxLength];
                                    memcpy(stateCorrect, state, sizeof(state));

                                    uint32_t buffer[TypeParam::MultiProjection::getRequiredBufferSize32B(5, 3)];
                                    typename TypeParam::MultiProjection& projection = TypeParam::MultiProjection::create(
                                            buffer,
                                            5, 3);
                                    if constexpr(printThings) printf("----------");
                                    uint32_t deltaPosition = 0;
                                    if(posA < posB) {
                                        if constexpr(printThings) printf(" <--%u:%u-->", posA, posB);
                                        memcpy(stateCorrect+posA, delta+deltaPosition, (posB-posA)*sizeof(*state));
                                        projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                                 posB - posA,
                                                                 {posA});
//                                        deltaPositionPosA = deltaPosition;
                                        deltaPosition += posB - posA;
                                    }
                                    if constexpr(printThings) printf(" [%u:%u](--)", posRecursiveA, posRecursiveA+2);
                                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                             2,
                                                             {posRecursiveA, 0});
                                    deltaPosition += 2;
                                    if(posC < posD) {
                                        if constexpr(printThings) printf(" <--%u:%u-->", posC, posD);
                                        memcpy(stateCorrect+posC, delta+deltaPosition, (posD-posC)*sizeof(*state));
                                        projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                                 posD - posC,
                                                                 {posC});
//                                        deltaPositionPosC = deltaPosition;
                                        deltaPosition += posD - posC;
                                    }
                                    if constexpr(printThings) printf(" [%u:%u](--)", posRecursiveB, posRecursiveB+2);
                                    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                             4,
                                                             {posRecursiveB, 0});
                                    deltaPosition += 4;
                                    if(posE < posF) {
                                        if constexpr(printThings) printf(" <--%u:%u-->", posE, posF);
                                        memcpy(stateCorrect+posE, delta+deltaPosition, (posF-posE)*sizeof(*state));
                                        projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE,
                                                                 posF - posE,
                                                                 {posE});
//                                        deltaPositionPosE = deltaPosition;
                                        deltaPosition += posF - posE;
                                    }
                                    if constexpr(printThings) printf("\n");

                                    auto newStateID = storage.delta(stateID.getState(), projection, false, delta);

                                    bool exists = storage.get(state2, newStateID.getState(), false);
                                    EXPECT_TRUE(exists);

                                    stateCorrect[posRecursiveA] = state2[posRecursiveA];
                                    stateCorrect[posRecursiveA+1] = state2[posRecursiveA+1];
                                    stateCorrect[posRecursiveB] = state2[posRecursiveB];
                                    stateCorrect[posRecursiveB+1] = state2[posRecursiveB+1];

                                    if(memcmp(stateCorrect, state2, sizeof(state2))) {
                                        printf("  Actual:");
                                        for(size_t i = 0; i < maxLength; ++i) {
                                            printf(" %02x", state2[i]);
                                        }
                                        printf("\n");
                                        printf("Expected:");
                                        for(size_t i = 0; i < maxLength; ++i) {
                                            printf(" %02x", stateCorrect[i]);
                                        }
                                        printf("\n");
                                        EXPECT_TRUE(false);
                                        abort();
                                    }

                                    typename TypeParam::StateSlot readBackDelta[maxLength*5];
                                    storage.getPartial(newStateID.getState(), projection, false, readBackDelta);

                                    if(memcmp(delta, readBackDelta, deltaPosition * sizeof(*state2))) {
                                        printf("  Actual:");
                                        for(size_t i = 0; i < deltaPosition; ++i) {
                                            printf(" %02x", readBackDelta[i]);
                                        }
                                        printf("\n");
                                        printf("Expected:");
                                        for(size_t i = 0; i < deltaPosition; ++i) {
                                            printf(" %02x", delta[i]);
                                        }
                                        printf("\n");
                                        EXPECT_TRUE(false);
                                        abort();
                                    }

                                }
                            }
                        }
                    }
                }
            }
        }
    }
    auto stats = storage.getStatistics();
    printf("_bytesReserved = %zu\n", stats._bytesReserved);
    printf("_bytesInUse = %zu\n", stats._bytesInUse);
    printf("_bytesMaximum = %zu\n", stats._bytesMaximum);
    printf("_elements = %zu\n", stats._elements);

}

TYPED_TEST(StorageTest, SparseGetStressTestRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
//        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    typename TypeParam::SparseOffset projection1[2];
                    projection1[0] = SparseOffset(posA, posB-posA);
                    projection1[1] = SparseOffset(posC, posD-posC);

                    storage.getSparse(stateID.getState(), state2, 2, projection1, true);

                    memcpy(stateCorrect          , state+posA, (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posB-posA, state+posC, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    if(memcmp(stateCorrect, state2, ((posB-posA) + (posD-posC)) * sizeof(typename TypeParam::StateSlot))) {
                        printf("  Actual:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseGetStressTestNonRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
//        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    typename TypeParam::SparseOffset projection1[2];
                    projection1[0] = SparseOffset(posA, posB-posA);
                    projection1[1] = SparseOffset(posC, posD-posC);

                    storage.getSparse(stateID.getState(), state2, 2, projection1, false);

                    memcpy(stateCorrect          , state+posA, (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posB-posA, state+posC, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    if(memcmp(stateCorrect, state2, ((posB-posA) + (posD-posC)) * sizeof(typename TypeParam::StateSlot))) {
                        printf("  Actual:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < ((posB-posA) + (posD-posC)); ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseDeltaStressTestRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    typename TypeParam::SparseOffset projection1[2];
                    projection1[0] = SparseOffset(posA, posB-posA);
                    projection1[1] = SparseOffset(posC, posD-posC);

                    auto newStateID = storage.deltaSparse(stateID.getState(), delta, 2, projection1, true);
                    if(posB != posC || (posA+1 == posB)) {
                        EXPECT_TRUE(newStateID.isInserted());
                    }

                    memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posA, delta          , (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posC, delta+posB-posA, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect)/sizeof(*stateCorrect), true);
                    EXPECT_TRUE(newStateIDFound.exists());

                    bool exists = storage.get(state2, newStateID.getState(), true);
                    EXPECT_TRUE(exists);
                    if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                        printf("  Actual:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                        abort();
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseDeltaStressTestNonRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 64;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA++) {
        for(uint32_t posB = posA+1; posB <= maxLength; posB++) {
            for(uint32_t posC = posB; posC <= maxLength; posC++) {
                for(uint32_t posD = posC+1; posD <= maxLength; posD++) {

                    typename TypeParam::SparseOffset projection1[2];
                    projection1[0] = SparseOffset(posA, posB-posA);
                    projection1[1] = SparseOffset(posC, posD-posC);

                    auto newStateID = storage.deltaSparse(stateID.getState(), delta, 2, projection1, false);

                    memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posA, delta          , (posB-posA) * sizeof(typename TypeParam::StateSlot));
                    memcpy(stateCorrect+posC, delta+posB-posA, (posD-posC) * sizeof(typename TypeParam::StateSlot));

                    auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect)/sizeof(*stateCorrect), false);
                    EXPECT_TRUE(newStateIDFound.exists());

                    bool exists = storage.get(state2, newStateID.getState(), false);
                    EXPECT_TRUE(exists);
                    if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                        printf("  Actual:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", state2[i]);
                        }
                        printf("\n");
                        printf("Expected:");
                        for(size_t i = 0; i < maxLength; ++i) {
                            printf(" %02x", stateCorrect[i]);
                        }
                        printf("\n");
                        EXPECT_TRUE(false);
                        abort();
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseDeltaStressTest3DRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 2048;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    const int INCR = 69;

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA+=INCR) {
        for(uint32_t posB = posA+1; posB <= maxLength && posB < posA+256; posB+=INCR) {
            for(uint32_t posC = posB; posC <= maxLength; posC+=INCR) {
                for(uint32_t posD = posC+1; posD <= maxLength && posD < posC+256; posD+=INCR) {
                    for(uint32_t posE = posD; posE <= maxLength; posE+=INCR) {
                        for(uint32_t posF = posE + 1; posF <= maxLength && posF < posE+256; posF+=INCR) {

                            typename TypeParam::SparseOffset projection1[3];
                            projection1[0] = SparseOffset(posA, posB - posA);
                            projection1[1] = SparseOffset(posC, posD - posC);
                            projection1[2] = SparseOffset(posE, posF - posE);

                            auto newStateID = storage.deltaSparse(stateID.getState(), delta, 3, projection1, true);

                            memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posA, delta, (posB - posA) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posC, delta + posB - posA, (posD - posC) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posE, delta + posB - posA + posD - posC, (posF - posE) * sizeof(typename TypeParam::StateSlot));

                            auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect) / sizeof(*stateCorrect), true);
                            EXPECT_TRUE(newStateIDFound.exists());

                            bool exists = storage.get(state2, newStateID.getState(), true);
                            EXPECT_TRUE(exists);
                            if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                                printf("A: %i\n", posA);
                                printf("B: %i\n", posB);
                                printf("C: %i\n", posC);
                                printf("D: %i\n", posD);
                                printf("E: %i\n", posE);
                                printf("F: %i\n", posF);
                                printf("  Actual:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", state2[i]);
                                }
                                printf("\n");
                                printf("Expected:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", stateCorrect[i]);
                                }
                                printf("\n");
                                EXPECT_TRUE(false);
                                abort();
                            }
                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseDeltaStressTest3DNonRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 512;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    const int INCR = 31;

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), false);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), false);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA+=INCR) {
        for(uint32_t posB = posA+1; posB <= maxLength && posB < posA+256; posB+=INCR) {
            for(uint32_t posC = posB; posC <= maxLength; posC+=INCR) {
                for(uint32_t posD = posC+1; posD <= maxLength && posD < posC+256; posD+=INCR) {
                    for(uint32_t posE = posD; posE <= maxLength; posE+=INCR) {
                        for(uint32_t posF = posE + 1; posF <= maxLength && posF < posE+256; posF+=INCR) {

                            typename TypeParam::SparseOffset projection1[3];
                            projection1[0] = SparseOffset(posA, posB - posA);
                            projection1[1] = SparseOffset(posC, posD - posC);
                            projection1[2] = SparseOffset(posE, posF - posE);

                            auto newStateID = storage.deltaSparse(stateID.getState(), delta, 3, projection1, false);

                            memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posA, delta, (posB - posA) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posC, delta + posB - posA, (posD - posC) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posE, delta + posB - posA + posD - posC, (posF - posE) * sizeof(typename TypeParam::StateSlot));

                            auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect) / sizeof(*stateCorrect), false);
                            EXPECT_TRUE(newStateIDFound.exists());

                            bool exists = storage.get(state2, newStateID.getState(), false);
                            EXPECT_TRUE(exists);
                            if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                                printf("A: %i\n", posA);
                                printf("B: %i\n", posB);
                                printf("C: %i\n", posC);
                                printf("D: %i\n", posD);
                                printf("E: %i\n", posE);
                                printf("F: %i\n", posF);
                                printf("  Actual:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", state2[i]);
                                }
                                printf("\n");
                                printf("Expected:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", stateCorrect[i]);
                                }
                                printf("\n");
                                EXPECT_TRUE(false);
                                abort();
                            }
                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseDeltaStressTest3DSmallRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 2048;
    typename TypeParam::StateSlot state[maxLength];
    typename TypeParam::StateSlot state2[maxLength];
    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot delta[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
        delta[i] = d;
    }

    const int INCR = 16;

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA+=INCR) {
        for(uint32_t posB = posA+1; posB <= maxLength && posB < posA+4; posB+=INCR) {
            for(uint32_t posC = posB; posC <= maxLength; posC+=INCR) {
                for(uint32_t posD = posC+1; posD <= maxLength && posD < posC+4; posD+=INCR) {
                    for(uint32_t posE = posD; posE <= maxLength; posE+=INCR) {
                        for(uint32_t posF = posE + 1; posF <= maxLength && posF < posE+4; posF+=INCR) {

                            typename TypeParam::SparseOffset projection1[3];
                            projection1[0] = SparseOffset(posA, posB - posA);
                            projection1[1] = SparseOffset(posC, posD - posC);
                            projection1[2] = SparseOffset(posE, posF - posE);

                            auto newStateID = storage.deltaSparse(stateID.getState(), delta, 3, projection1, true);

                            memcpy(stateCorrect, state, maxLength * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posA, delta, (posB - posA) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posC, delta + posB - posA, (posD - posC) * sizeof(typename TypeParam::StateSlot));
                            memcpy(stateCorrect + posE, delta + posB - posA + posD - posC, (posF - posE) * sizeof(typename TypeParam::StateSlot));

                            auto newStateIDFound = storage.find(stateCorrect, sizeof(stateCorrect) / sizeof(*stateCorrect), true);
                            EXPECT_TRUE(newStateIDFound.exists());

                            bool exists = storage.get(state2, newStateID.getState(), true);
                            EXPECT_TRUE(exists);
                            if(memcmp(stateCorrect, state2, (maxLength) * sizeof(typename TypeParam::StateSlot))) {
                                printf("A: %i\n", posA);
                                printf("B: %i\n", posB);
                                printf("C: %i\n", posC);
                                printf("D: %i\n", posD);
                                printf("E: %i\n", posE);
                                printf("F: %i\n", posF);
                                printf("  Actual:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", state2[i]);
                                }
                                printf("\n");
                                printf("Expected:");
                                for(size_t i = 0; i < maxLength; ++i) {
                                    printf(" %02x", stateCorrect[i]);
                                }
                                printf("\n");
                                EXPECT_TRUE(false);
                                abort();
                            }
                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, SparseGetAndModifyStressTest3DSmallRoot) {
    TypeParam storage;

    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 2048;
    typename TypeParam::StateSlot state[maxLength];
//    typename TypeParam::StateSlot state2[maxLength];
//    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot buffer[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
    }

    const int INCR = 16;

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA+=INCR) {
        for(uint32_t posB = posA+1; posB <= maxLength && posB < posA+4; posB+=INCR) {
            for(uint32_t posC = posB; posC <= maxLength; posC+=INCR) {
                for(uint32_t posD = posC+1; posD <= maxLength && posD < posC+4; posD+=INCR) {
                    for(uint32_t posE = posD; posE <= maxLength; posE+=INCR) {
                        for(uint32_t posF = posE + 1; posF <= maxLength && posF < posE+4; posF+=INCR) {

                            typename TypeParam::SparseOffset projection1[3];
                            projection1[0] = SparseOffset(posA, posB - posA);
                            projection1[1] = SparseOffset(posC, posD - posC);
                            projection1[2] = SparseOffset(posE, posF - posE);

                            storage.getSparse(stateID.getState(), buffer, 3, projection1, true);
                            uint32_t bufferLength = posB - posA + posD - posC + posF - posE;

                            for(size_t i = 0; i < bufferLength; ++i) {
                                buffer[i]++;
                            }

                            storage.deltaSparse(stateID.getState(), buffer, 3, projection1, true);

                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, GetAndModifyStressTest3DSmallRoot) {
    TypeParam storage;

//    using SparseOffset = typename TypeParam::SparseOffset;

    const size_t maxLength = 2048;
    typename TypeParam::StateSlot state[maxLength];
//    typename TypeParam::StateSlot state2[maxLength];
//    typename TypeParam::StateSlot stateCorrect[maxLength];
    typename TypeParam::StateSlot buffer[maxLength];

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(maxLength);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    char c = 'A';
    char d = '0';

    for(size_t i = 0; i < maxLength; ++i, ++c, ++d) {
        if(c == '[') c = 'A';
        if(d == ':') d = '0';
        state[i] = c;
    }

    const int INCR = 16;

    auto stateID = storage.insert(state, sizeof(state)/sizeof(*state), true);
    auto stateIDFound = storage.find(state, sizeof(state)/sizeof(*state), true);
    EXPECT_TRUE(stateIDFound.exists());

    for(uint32_t posA = 0; posA < maxLength; posA+=INCR) {
        for(uint32_t posB = posA+1; posB <= maxLength && posB < posA+4; posB+=INCR) {
            for(uint32_t posC = posB; posC <= maxLength; posC+=INCR) {
                for(uint32_t posD = posC+1; posD <= maxLength && posD < posC+4; posD+=INCR) {
                    for(uint32_t posE = posD; posE <= maxLength; posE+=INCR) {
                        for(uint32_t posF = posE + 1; posF <= maxLength && posF < posE+4; posF+=INCR) {

                            uint32_t bufferLength = posF - posA;

                            storage.getPartial(stateID.getState(), posA, buffer, bufferLength, true);

                            for(size_t i = 0; i < posB-posA; ++i) {
                                buffer[i]++;
                            }
                            for(size_t i = posC-posA; i < posD-posA; ++i) {
                                buffer[i]++;
                            }
                            for(size_t i = posE-posA; i < posF-posA; ++i) {
                                buffer[i]++;
                            }

                            storage.insert(stateID.getState(), posA, bufferLength, buffer, true);

                        }
                    }
                }
            }
        }
    }
}

TYPED_TEST(StorageTest, PartialGetOne) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {1, 2, 3, 4, 5, 6, 7, 8};
    size_t nrSlots = sizeof(state1) / sizeof(*state1);

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(nrSlots);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    // State 1
    stateID= storage.insert(state1, nrSlots, true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, nrSlots, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    for(int offset = 0; offset < 8; ++offset) {
        typename TypeParam::StateSlot d = 0;
        storage.getPartial(stateID.getState(), offset, &d, 1, true);
        EXPECT_EQ(d, state1[offset]);
    }

}

TYPED_TEST(StorageTest, PartialGet) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3, 4, 5, 6, 7};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot partial[2];

    storage.getPartial(stateID.getState(), 4, partial, 2, true);
    EXPECT_EQ(partial[0], state1[4]);
    EXPECT_EQ(partial[1], state1[5]);
}

TYPED_TEST(StorageTest, PartialGetMultiProjection) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3, 4, 5, 6, 7};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot partial[2] = {0};

    uint32_t buffer[TypeParam::MultiProjection::getRequiredBufferSize32B(2,2)];
    typename TypeParam::MultiProjection& projection = TypeParam::MultiProjection::create(buffer, 2, 2);
    projection.addProjection(TypeParam::MultiOffset::Options::READ_WRITE, 2, {1});

    printf("partial = %p\n", partial);

    storage.getPartial(stateID.getState(), projection, true, partial);
    EXPECT_EQ(partial[0], state1[1]);
    EXPECT_EQ(partial[1], state1[2]);
}

TYPED_TEST(StorageTest, PartialGetOfVector2) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {1, 0};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot partial[2];

    storage.get(partial, stateID.getState(), true);
    EXPECT_EQ(partial[0], 1);
    EXPECT_EQ(partial[1], 0);
}

TYPED_TEST(StorageTest, PartialGetOfVector2NoRoot) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {1, 0};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), false);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), false);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot partial[2];

    storage.get(partial, stateID.getState(), false);
    EXPECT_EQ(partial[0], 1);
    EXPECT_EQ(partial[1], 0);
}

TYPED_TEST(StorageTest, DeltaVectorLength2) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {2, 0};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), true);

    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot d = 1;
    auto delta = TypeParam::Delta::create(0,&d, 1);
    auto newStateID = storage.insert(stateID.getState(), *delta, true);

    EXPECT_TRUE(newStateID.isInserted());

    typename TypeParam::StateSlot partial[2];

    storage.get(partial, newStateID.getState(), true);
    EXPECT_EQ(partial[0], 1);
    EXPECT_EQ(partial[1], 0);
}

TYPED_TEST(StorageTest, DeltaVectorLength2NoRoot) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = {0, 0};
    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(state1)/sizeof(*state1));
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    stateID = storage.insert(state1, sizeof(state1)/sizeof(*state1), false);

    if(std::is_same<TypeParam, llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>>::value) {
        EXPECT_EQ(stateID.getState().getData() & 0xFFFFFFFFFFULL, 0);
    }

    stateIDFound = storage.find(state1, sizeof(state1)/sizeof(*state1), false);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot d = 1;
    auto delta = TypeParam::Delta::create(0,&d, 1);
    auto newStateID = storage.insert(stateID.getState(), *delta, false);

    typename TypeParam::StateSlot partial[2];

    storage.get(partial, newStateID.getState(), false);
    EXPECT_EQ(partial[0], 1);
    EXPECT_EQ(partial[1], 0);
}

struct Proc {
    uint32_t id;
    uint32_t max;
    uint32_t pc;
    uint32_t j;
};

struct SV {
    int procs;
    uint32_t state;
    uint32_t choosing[4];
    uint32_t number[4];
    Proc proc[4];
};

TYPED_TEST(StorageTest, Bla) {
    TypeParam storage;

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(sizeof(SV)/4);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    typename TypeParam::StateSlot svmem[sizeof(SV) / 4];
    memset(svmem, 0, sizeof(SV));
    SV& sv = *(SV*)svmem;
    sv.state    = 3;
    sv.procs    = 4;

    //Proc::MAGIC_MAX = procs+1;  // magic!
    for (auto i = 4; i-- ;) {
        Proc& p = sv.proc[i];
        p.id = i;
        p.max = p.pc = p.j = 0;
    }

    auto stateID = storage.insert(svmem, sizeof(SV)/4, true);

    auto stateIDFound = storage.find(svmem, sizeof(SV)/4, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot svmem2[sizeof(SV) / 4];
    memset(svmem2, 0, sizeof(SV));
    storage.get(svmem2, stateID.getState(), true);
    assert(!memcmp(svmem, svmem2, sizeof(SV)));

}

TYPED_TEST(StorageTest, InsertLength35) {
    TypeParam storage;

    typename TypeParam::StateSlot state1[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'
                                             , 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'
                                             , 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3'
                                             , '4', '5', '6', '7', '8'
                                             };
//    typename TypeParam::StateSlot state1[8] = {0};
    size_t slots = sizeof(state1)/sizeof(*state1);

    if constexpr(TypeParam::stateHasFixedLength()) {
        storage.setMaxStateLength(slots);
    }
    storage.init();
    if constexpr(TypeParam::needsThreadInit()) {
        storage.thread_init();
    }

    auto stateID = storage.insert(state1, slots, true);
    EXPECT_TRUE(stateID.isInserted());

    auto stateIDFound = storage.find(state1, slots, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    auto fsd = storage.get(stateID.getState(), true);
    assert(fsd);
    EXPECT_FALSE(memcmp(state1, fsd->getData(), slots * sizeof(typename TypeParam::StateSlot)));

    typename TypeParam::StateSlot readback[slots];

    storage.get(readback, stateID.getState(), true);
    EXPECT_FALSE(memcmp(state1, readback, slots * sizeof(typename TypeParam::StateSlot)));
}

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}