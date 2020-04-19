#include <type_traits>

#include <llmc/storage/stdmap.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>
#include <gtest/gtest.h>
#include <gtest/gtest-typed-test.h>
#include <llmc/storage/cchm.h>

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

using MyTypes = ::testing::Types< llmc::storage::TreeDBSStorage<llmc::storage::StdMap>
                                , llmc::storage::TreeDBSStorageModified
                                , llmc::storage::StdMap
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
    }
}

TYPED_TEST(StorageTest, Insert2to1024nonroot) {
    TypeParam storage;

    const size_t maxLength = 1024;
    typename TypeParam::StateSlot state[maxLength];

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

    for(int length = 2; length < nrSlots; length++) {

        // State 1
        stateIDs[1] = storage.insert(state1, length, true);
        EXPECT_TRUE(stateIDs[1].isInserted());
        stateIDFound = storage.find(state1, length, true);
        EXPECT_TRUE(stateIDFound.exists());
        EXPECT_EQ(stateIDs[1].getState(), stateIDFound);

        for(int offset = 0; offset < length; ++offset) {
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

    stateID = storage.insert(state1, 8, true);
    EXPECT_TRUE(stateID.isInserted());
    stateIDFound = storage.find(state1, 8, true);
    EXPECT_TRUE(stateIDFound.exists());
    EXPECT_EQ(stateID.getState(), stateIDFound);

    typename TypeParam::StateSlot partial[2];

    storage.getPartial(stateID.getState(), 4, partial, 2, true);
    EXPECT_EQ(partial[0], state1[4]);
    EXPECT_EQ(partial[1], state1[5]);
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

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}