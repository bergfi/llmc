#include <llmc/storage/interface.h>
#include <llmc/storage/dtree.h>
#include <gtest/gtest.h>
#include <gtest/gtest-typed-test.h>

template <typename T>
class FooTest : public ::testing::Test {
public:
    T value_;
};

using MyTypes = ::testing::Types<CrappyStorage, DTreeStorage<SeparateRootSingleHashSet<HashSet<RehasherExit>>>>;
//using MyTypes = ::testing::Types<DTreeStorage<SeparateRootSingleHashSet<HashSet<RehasherExit>>>>;
TYPED_TEST_CASE(FooTest, MyTypes);

TYPED_TEST(FooTest, BasicInsertLength2) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot states[] = {0, 1, 2, 3, 4, 5};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[6];

    stateIDs[1] = storage.insert(&states[1], 2, true);
    stateIDs[2] = storage.insert(&states[2], 2, true);
    stateIDs[3] = storage.insert(&states[3], 2, false);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDs[3].isInserted());

    stateIDsFound[1] = storage.find(&states[1], 2, true);
    stateIDsFound[2] = storage.find(&states[2], 2, true);
    stateIDsFound[3] = storage.find(&states[3], 2, false);
    stateIDsFound[4] = storage.find(&states[4], 2, true);
    stateIDsFound[5] = storage.find(&states[3], 2, true);

    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_FALSE(stateIDsFound[4].exists());
    EXPECT_FALSE(stateIDsFound[5].exists());

    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
}

TYPED_TEST(FooTest, BasicInsertLength4) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot states[] = {272, 273, 274, 275, 276, 277, 278, 279};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[6];

    stateIDs[1] = storage.insert(&states[1], 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());

    stateIDs[2] = storage.insert(&states[2], 4, true);
    EXPECT_TRUE(stateIDs[2].isInserted());

    stateIDs[3] = storage.insert(&states[3], 4, false);
    EXPECT_TRUE(stateIDs[3].isInserted());

    stateIDsFound[1] = storage.find(&states[1], 4, true);
    stateIDsFound[2] = storage.find(&states[2], 4, true);
    stateIDsFound[3] = storage.find(&states[3], 4, false);
    stateIDsFound[4] = storage.find(&states[4], 4, true);
    stateIDsFound[5] = storage.find(&states[3], 4, true);

    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_FALSE(stateIDsFound[4].exists());
    EXPECT_FALSE(stateIDsFound[5].exists());

    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
}

TYPED_TEST(FooTest, BasicDuplicateInsert) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot states[] = {0, 1, 2, 3, 4, 5};
    typename TypeParam::InsertedState stateIDs[6];
    typename TypeParam::StateID stateIDsFound;

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
    EXPECT_NE(stateIDs[4].getState(), stateIDs[3].getState());

    // Find the duplicate
    stateIDsFound = storage.find(&states[0], 4, true);
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound);
}

TYPED_TEST(FooTest, BasicDeltaTransition) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::InsertedState stateIDs[2];
    typename TypeParam::StateID stateIDsFound;

    stateIDs[0] = storage.insert(state1, 4, true);
    stateIDsFound = storage.find(state1, 4, true);
    EXPECT_TRUE(stateIDs[0].isInserted());
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[0].getState(), stateIDsFound);

    typename TypeParam::Delta* delta = TypeParam::Delta::create(0, state2, 4);
    stateIDs[1] = storage.insert(stateIDs[0].getState(), *delta, true);
    stateIDsFound = storage.find(state2, 4, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDsFound.exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound);
    TypeParam::Delta::destroy(delta);
}

TYPED_TEST(FooTest, BasicLoopTransition) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot states[] = {0, 1, 2, 3};
    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

    stateIDs[1] = storage.insert(&states[1], 2, true);
    stateIDsFound[1] = storage.find(&states[1], 2, true);
    EXPECT_TRUE(stateIDs[1].isInserted());
    EXPECT_TRUE(stateIDsFound[1].exists());
    EXPECT_EQ(stateIDs[1].getState(), stateIDsFound[1]);

    typename TypeParam::Delta* delta1 = TypeParam::Delta::create(0, &states[2], 2);
    stateIDs[2] = storage.insert(stateIDs[1].getState(), *delta1, true);
    stateIDsFound[2] = storage.find(&states[2], 2, true);
    EXPECT_TRUE(stateIDs[2].isInserted());
    EXPECT_TRUE(stateIDsFound[2].exists());
    EXPECT_EQ(stateIDs[2].getState(), stateIDsFound[2]);
    TypeParam::Delta::destroy(delta1);

    typename TypeParam::Delta* delta2 = TypeParam::Delta::create(0, &states[1], 2);
    stateIDs[3] = storage.insert(stateIDs[2].getState(), *delta2, true);
    stateIDsFound[3] = storage.find(&states[1], 2, true);
    EXPECT_FALSE(stateIDs[3].isInserted());
    EXPECT_TRUE(stateIDsFound[3].exists());
    EXPECT_EQ(stateIDs[3].getState(), stateIDsFound[3]);
    EXPECT_EQ(stateIDs[3].getState(), stateIDs[1].getState());
    TypeParam::Delta::destroy(delta2);
}

TYPED_TEST(FooTest, MultiSlotGet) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};

    typename TypeParam::InsertedState stateID;
    typename TypeParam::StateID stateIDFound;

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

TYPED_TEST(FooTest, MultiSlotLoopTransition) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::StateSlot state3[] = {0, 1, 2, 3};

    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

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

TYPED_TEST(FooTest, MultiSlotLoopTransitionWithOffset) {
    TypeParam storage;
    storage.init();

    typename TypeParam::StateSlot state1[] = {0, 1, 2, 3};
    typename TypeParam::StateSlot state2[] = {0, 1, 2, 4};
    typename TypeParam::StateSlot state3[] = {0, 1, 2, 3};

    typename TypeParam::InsertedState stateIDs[4];
    typename TypeParam::StateID stateIDsFound[4];

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

TYPED_TEST(FooTest, MultiSlotDiamondTransition) {
    TypeParam storage;
//    storage.setRootScale(8);
//    storage.setScale(8);
    storage.init();

    typename TypeParam::StateSlot state1[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    typename TypeParam::StateSlot state2[] = {0x11111111, 0x22222222, 0x33333333, 0x99999999};
    typename TypeParam::StateSlot state3[] = {0x88888888, 0x22222222, 0x33333333, 0x44444444};
    typename TypeParam::StateSlot state4[] = {0x88888888, 0x22222222, 0x33333333, 0x99999999};

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

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}