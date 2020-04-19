#include <llmc/modelcheckers/singlecore.h>
#include <llmc/modelcheckers/multicore.h>
#include <llmc/modelcheckers/multicoresimple.h>
#include <llmc/storage/stdmap.h>
#include <llmc/storage/dtree.h>
#include <llmc/storage/treedbs.h>
#include <llmc/storage/treedbsmod.h>
#include <gtest/gtest.h>
#include <gtest/gtest-typed-test.h>
#include <llmc/storage/cchm.h>
#include <fstream>

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
class ModelInterfaceTest: public ::testing::Test {
public:
    using MyType = T;
};

using MyTypes = ::testing::Types< SingleCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::StdMap, llmc::statespace::StatsGatherer>
        , SingleCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::cchm, llmc::statespace::StatsGatherer>
//        , SingleCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, llmc::statespace::StatsGatherer>
        , SingleCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::TreeDBSStorageModified, llmc::statespace::StatsGatherer>
        , SingleCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, llmc::statespace::StatsGatherer>
        //, MultiCoreModelCheckerSimple<VModel<llmc::storage::StorageInterface>, llmc::storage::TreeDBSStorage<llmc::storage::StdMap>, llmc::statespace::StatsGatherer>
        , MultiCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::TreeDBSStorageModified, llmc::statespace::StatsGatherer>
        , MultiCoreModelChecker<VModel<llmc::storage::StorageInterface>, llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, llmc::statespace::StatsGatherer>
        , MultiCoreModelCheckerSimple<VModel<llmc::storage::StorageInterface>, llmc::storage::TreeDBSStorageModified, llmc::statespace::StatsGatherer>
        , MultiCoreModelCheckerSimple<VModel<llmc::storage::StorageInterface>, llmc::storage::DTreeStorage<SeparateRootSingleHashSet<HashSet128<RehasherExit, QuadLinear, HashCompareMurmur>, HashSet<RehasherExit, QuadLinear, HashCompareMurmur>>>, llmc::statespace::StatsGatherer>
>;

TYPED_TEST_CASE(ModelInterfaceTest, MyTypes);

template<int SLOTMAX, int VLENGTH, int ELENGTH>
class SimpleIJModel: public VModel<llmc::storage::StorageInterface> {
public:
    size_t getNextAll(StateID const& s, Context* ctx) override {
        auto state = ctx->getModelChecker()->getState(ctx, s);
        assert(state);
//        StateSlot d[state->getLength()];

        size_t buffer[2 + ELENGTH];

        size_t imax = state->getLength();
        for(size_t i = 0; i < imax; i += ELENGTH) {
            StateSlot diff[ELENGTH] = {0};
            diff[0] = (state->getData()[i] + 1) % SLOTMAX;
            ctx->getModelChecker()->newTransition(ctx, llmc::storage::StorageInterface::Delta::create((llmc::storage::StorageInterface::Delta*)buffer, i, diff, ELENGTH), TransitionInfoUnExpanded::None());
        }
        return state->getLength();
    }

    size_t getNext(StateID const& s, Context* ctx, size_t tg) override {
        auto state = ctx->getModelChecker()->getState(ctx, s);
        assert(state);
//        StateSlot d[state->getLength()];

        size_t buffer[2 + ELENGTH];

//        size_t imax = state->getLength();

        StateSlot diff[ELENGTH] = {0};
        diff[0] = (state->getData()[tg * ELENGTH] + 1) % SLOTMAX;
        ctx->getModelChecker()->newTransition(ctx, llmc::storage::StorageInterface::Delta::create((llmc::storage::StorageInterface::Delta*)buffer, tg * ELENGTH, diff, ELENGTH), TransitionInfoUnExpanded::None());

        return state->getLength();
    }

    StateID getInitial(Context* ctx) override {
        StateSlot d[VLENGTH] = {0};
        return ctx->getModelChecker()->newState(ctx, 0, sizeof(d)/sizeof(*d), d).getState();
    }

    llmc::statespace::Type* getStateVectorType() override {
        return nullptr;
    }

    void init(Context* ctx) override {
    }

    TransitionInfo getTransitionInfo(VContext<llmc::storage::StorageInterface>* ctx,
                                     TransitionInfoUnExpanded const& tinfo_) const override {
        return TransitionInfo();
    }

    size_t getStateLength() const override {
        return 0;
    }
};

TYPED_TEST(ModelInterfaceTest, SimpleIJTest10_4_4) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<10, 4, 4>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(4);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(10, mc.getStates());
    EXPECT_EQ(10, mc.getTransitions());
}

TYPED_TEST(ModelInterfaceTest, SimpleIJTest10_8_4) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<10, 8, 4>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(8);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(100, mc.getStates());
    EXPECT_EQ(200, mc.getTransitions());
}

TYPED_TEST(ModelInterfaceTest, SimpleIJTest10_16_4) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<10, 16, 4>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(16);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(10000, mc.getStates());
    EXPECT_EQ(40000, mc.getTransitions());
}

TYPED_TEST(ModelInterfaceTest, SimpleIJTest10_80_16) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<10, 80, 16>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(80);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(100000, mc.getStates());
    EXPECT_EQ(500000, mc.getTransitions());
}

TYPED_TEST(ModelInterfaceTest, SimpleIJTest10_96_16) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<10, 96, 16>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(96);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(1000000, mc.getStates());
    EXPECT_EQ(6000000, mc.getTransitions());
}

TYPED_TEST(ModelInterfaceTest, DISABLED_SimpleIJTest8_144_16) {
    Settings& settings = Settings::global();
    auto model = new SimpleIJModel<8, 144, 16>();
    typename TypeParam::Listener listener;
    TypeParam mc(model, listener);
    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
        mc.getStorage().setMaxStateLength(144);
    }
    mc.getStorage().setSettings(settings);
    mc.setSettings(settings);
    mc.go();
    EXPECT_EQ(134217728, mc.getStates());
    EXPECT_EQ(1207959552, mc.getTransitions());
}

//TYPED_TEST(ModelInterfaceTest, SimpleIJTest8_96_16) {
//    auto model = new SimpleIJModel<8, 96, 16>();
//    llmc::statespace::StatsGatherer< TypeParam
//            , VModel<llmc::storage::StorageInterface>
//    > statsGatherer;
//    TypeParam mc(model, statsGatherer);
//    if constexpr(std::remove_reference<decltype(mc.getStorage())>::type::stateHasFixedLength()) {
//        mc.getStorage().setMaxStateLength(96);
//    }
//    mc.go();
//    EXPECT_EQ(262144, statsGatherer.getStates());
//    EXPECT_EQ(262144*6, statsGatherer.getTransitions());
//}

int main(int argc, char** argv) {

    Settings& settings = Settings::global();
    int c = 0;
    while ((c = getopt(argc, argv, "m:s:t:-:")) != -1) {
//    while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
        switch(c) {
            case 't':
                if(optarg) {
                    settings["threads"] = std::stoi(optarg);
                }
                break;
            case 'm':
                if(optarg) {
                    settings["mc"] = std::string(optarg);
                }
                break;
            case 's':
                if(optarg) {
                    settings["storage"] = std::string(optarg);
                }
                break;
            case '-':
                settings.insertKeyValue(optarg);
        }
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}