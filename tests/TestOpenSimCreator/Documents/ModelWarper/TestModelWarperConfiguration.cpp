#include <OpenSimCreator/Documents/ModelWarper/ModelWarperConfiguration.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/TemporaryFile.h>

#include <filesystem>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::filesystem::path GetFixturePath(const std::filesystem::path& subpath)
    {
        return std::filesystem::weakly_canonical(std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / subpath);
    }
}

static_assert(StrategyMatchQuality::none() < StrategyMatchQuality::wildcard());
static_assert(StrategyMatchQuality::wildcard() < StrategyMatchQuality::exact());
static_assert(static_cast<bool>(StrategyMatchQuality::none()) == false);
static_assert(static_cast<bool>(StrategyMatchQuality::wildcard()) == true);
static_assert(static_cast<bool>(StrategyMatchQuality::exact()) == true);

TEST(RuntimeWarpParameters, ConstructedWithBlendFactorMakesGetBlendFactorReturnTheBlendFactor)
{
    RuntimeWarpParameters params{0.3f};
    ASSERT_EQ(params.getBlendFactor(), 0.3f);
}

TEST(WarpCache, CanDefaultConstruct)
{
    [[maybe_unused]] WarpCache instance;
}

TEST(ModelWarperConfiguration, CanDefaultConstruct)
{
    [[maybe_unused]] ModelWarperConfiguration instance;
}

TEST(IdentityComponentWarper, CanDefaultConstruct)
{
    [[maybe_unused]] IdentityComponentWarper instance;
}

TEST(IdentityComponentWarper, DoesNotChangeAnyComponentProperty)
{
    OpenSim::Model sourceModel;
    OpenSim::Marker& sourceMarker = AddMarker(sourceModel, "marker", sourceModel.getGround(), SimTK::Vec3{0.0});
    FinalizeConnections(sourceModel);
    InitializeModel(sourceModel);
    OpenSim::Model destinationModel = sourceModel;  // create copy for writing
    InitializeModel(destinationModel);
    auto& destinationMarker = sourceModel.updComponent<OpenSim::Marker>(sourceMarker.getAbsolutePath());

    RuntimeWarpParameters parameters;
    WarpCache cache;
    IdentityComponentWarper warper;

    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
    warper.warpInPlace(parameters, cache, sourceModel, sourceMarker, destinationModel, destinationMarker);
    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
}

TEST(ExceptionThrowingComponentWarper, ThrowsWhenWarpInPlaceIsCalled)
{
    OpenSim::Model sourceModel;
    OpenSim::Marker& sourceMarker = AddMarker(sourceModel, "marker", sourceModel.getGround(), SimTK::Vec3{0.0});
    FinalizeConnections(sourceModel);
    InitializeModel(sourceModel);
    OpenSim::Model destinationModel = sourceModel;  // create copy for writing
    InitializeModel(destinationModel);
    auto& destinationMarker = sourceModel.updComponent<OpenSim::Marker>(sourceMarker.getAbsolutePath());

    RuntimeWarpParameters parameters;
    WarpCache cache;
    ExceptionThrowingComponentWarper warper{"some message content"};

    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
    bool warperThrownException = false;
    try {
        warper.warpInPlace(parameters, cache, sourceModel, sourceMarker, destinationModel, destinationMarker);
    } catch (const std::exception& ex) {
        ASSERT_TRUE(contains(ex.what(), "some message content"));
        warperThrownException = true;
    }
    ASSERT_TRUE(warperThrownException) << "should always throw an exception";
    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
}

TEST(ModelWarperConfiguration, CanSaveAndLoadDefaultConstructedToAndFromXMLFile)
{
    TemporaryFile temporaryFile;
    temporaryFile.close();  // so that `OpenSim::Object::print`'s implementation can open+write to it

    ModelWarperConfiguration configuration;
    configuration.print(temporaryFile.absolute_path().string());

    ModelWarperConfiguration loadedConfiguration{temporaryFile.absolute_path().string()};
    loadedConfiguration.finalizeFromProperties();
    loadedConfiguration.finalizeConnections(loadedConfiguration);
}

TEST(ModelWarperConfiguration, LoadingNonExistentFileThrows)
{
    ASSERT_ANY_THROW({ [[maybe_unused]] ModelWarperConfiguration configuration{GetFixturePath("doesnt_exist")}; });
}

TEST(ModelWarperConfiguration, CanLoadEmptySequence)
{
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/empty_sequence.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleOffsetFrameWarpingStrategy)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/single_offsetframe_warper.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    auto range = configuration.getComponentList<ProduceErrorOffsetFrameWarpingStrategy>();
    const auto numEls = std::distance(range.begin(), range.end());
    ASSERT_EQ(numEls, 1);
}

TEST(ModelWarperConfiguration, CanContainAMixtureOfOffsetFrameWarpingStrategies)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/mixed_offsetframe_warpers.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    auto range = configuration.getComponentList<OffsetFrameWarpingStrategy>();
    const auto numEls = std::distance(range.begin(), range.end());
    ASSERT_EQ(numEls, 2);
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleStationWarpingStrategy)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/single_station_warper.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    auto range = configuration.getComponentList<ProduceErrorStationWarpingStrategy>();
    const auto numEls = std::distance(range.begin(), range.end());
    ASSERT_EQ(numEls, 1);
}

TEST(ModelWarperConfiguration, CanLoadAMixtureOfStationWarpingStrategies)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/mixed_station_warpers.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    auto range = configuration.getComponentList<StationWarpingStrategy>();
    const auto numEls = std::distance(range.begin(), range.end());
    ASSERT_EQ(numEls, 2);
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesFailsIfNoStrategyTargets)
{
    ProduceErrorOffsetFrameWarpingStrategy strategy;
    ASSERT_ANY_THROW({ strategy.finalizeFromProperties(); }) << "should fail, because the strategy has no targets (ambiguous definition)";
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesWorksIfThereIsAStrategyTarget)
{
    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("*");
    ASSERT_NO_THROW({ strategy.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, LoadingConfigurationContainingStrategyWithTwoTargetsWorksAsExpected)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/two_strategy_targets.xml")};
    configuration.finalizeFromProperties();

    const auto* strategy = configuration.findComponent<ProduceErrorStationWarpingStrategy>("two_targets");
    ASSERT_EQ(strategy->getProperty_StrategyTargets().size(), 2);
    ASSERT_EQ(strategy->get_StrategyTargets(0), "/first/target");
    ASSERT_EQ(strategy->get_StrategyTargets(1), "*");
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesThrowsIfDuplicateStrategyTargetsDetected)
{
    // note: this validation check might be relied upon by the validation passes of
    // higher-level components (e.g. `ModelWarperConfiguration`)

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("/some/target");
    strategy.append_StrategyTargets("/some/target");

    ASSERT_ANY_THROW({ strategy.finalizeFromProperties(); }) << "finalizeFromProperties should throw if duplicate StrategyTargets are declared";
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesThrowsIfDuplicateWildcardStrategyTargetsDetected)
{
    // note: this validation check might be relied upon by the validation passes of
    // higher-level components (e.g. `ModelWarperConfiguration`)

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("*");
    strategy.append_StrategyTargets("*");

    ASSERT_ANY_THROW({ strategy.finalizeFromProperties(); }) << "finalizeFromProperties should throw if duplicate StrategyTargets are declared (even wildcards)";
}

TEST(ModelWarperConfiguration, finalizeFromPropertiesThrowsWhenGivenConfigurationContainingTwoStrategiesWithTheSameStrategyTarget)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/duplicated_offsetframe_strategytarget.xml")};

    ASSERT_ANY_THROW({ configuration.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, finalizeFromPropertiesDoesNotThrowWhenGivenConfigurationContainingTwoDifferentTypesOfStrategiesWithTheSameStrategyTarget)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/duplicated_but_different_types.xml")};

    ASSERT_NO_THROW({ configuration.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, MatchingAnOffsetFrameStrategyToExactPathWorksAsExpected)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("/someoffsetframe");
    strategy.finalizeConnections(strategy);

    ASSERT_EQ(strategy.calculateMatchQuality(pof), StrategyMatchQuality::exact());
}

TEST(ModelWarperConfiguration, MatchingAnOffsetFrameStrategyToWildcardWorksAsExpected)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("*");
    strategy.finalizeConnections(strategy);

    ASSERT_EQ(strategy.calculateMatchQuality(pof), StrategyMatchQuality::wildcard());
}

TEST(ModelWarperConfiguration, MatchesExactlyEvenIfWildcardMatchIsAlsoPresent)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("*");
    strategy.append_StrategyTargets("/someoffsetframe");  // should match this
    strategy.finalizeConnections(strategy);

    ASSERT_EQ(strategy.calculateMatchQuality(pof), StrategyMatchQuality::exact());
}

TEST(ModelWarperConfiguration, MatchesWildcardIfInvalidPathPresent)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("/someinvalidpath");
    strategy.append_StrategyTargets("*");  // should match this, because the exact one isn't valid for the component
    strategy.finalizeConnections(strategy);

    ASSERT_EQ(strategy.calculateMatchQuality(pof), StrategyMatchQuality::wildcard());
}

TEST(ModelWarperConfiguration, MatchesMoreSpecificStrategyWhenTwoStrategiesAreAvailable)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ModelWarperConfiguration configuration;
    // add less-specific strategy
    {
        auto strategy = std::make_unique<ProduceErrorOffsetFrameWarpingStrategy>();
        strategy->append_StrategyTargets("*");  // should match this, because the exact one isn't valid for the component
        configuration.addComponent(strategy.release());
    }
    // add more-specific one
    {
        auto strategy = std::make_unique<IdentityOffsetFrameWarpingStrategy>();
        strategy->append_StrategyTargets("/someoffsetframe");
        configuration.addComponent(strategy.release());
    }
    configuration.finalizeConnections(configuration);

    const ComponentWarpingStrategy* matchedStrategy = configuration.tryMatchStrategy(pof);

    ASSERT_NE(matchedStrategy, nullptr);
    ASSERT_NE(dynamic_cast<const IdentityOffsetFrameWarpingStrategy*>(matchedStrategy), nullptr);
}

TEST(ModelWarperConfiguration, tryMatchStrategyDoesNotThrowIfTwoWildcardsForDifferentTargetsMatch)
{
    OpenSim::Model model;
    OpenSim::PhysicalOffsetFrame& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
        model,
        "someoffsetframe",
        model.getGround(),
        SimTK::Transform{}
    );
    model.finalizeConnections();
    ASSERT_EQ(pof.getAbsolutePathString(), "/someoffsetframe");

    ModelWarperConfiguration configuration;
    // add a wildcard strategy specifically for `OpenSim::Station`
    {
        auto strategy = std::make_unique<ProduceErrorStationWarpingStrategy>();
        strategy->append_StrategyTargets("*");
        configuration.addComponent(strategy.release());
    }
    // add a wildcard strategy specifically for `OpenSim::PhysicalOffsetFrame`
    {
        auto strategy = std::make_unique<ProduceErrorOffsetFrameWarpingStrategy>();
        strategy->append_StrategyTargets("*");
        configuration.addComponent(strategy.release());
    }
    configuration.finalizeConnections(configuration);

    const ComponentWarpingStrategy* matchedStrategy = configuration.tryMatchStrategy(pof);

    ASSERT_NE(matchedStrategy, nullptr);
    ASSERT_NE(dynamic_cast<const ProduceErrorOffsetFrameWarpingStrategy*>(matchedStrategy), nullptr);
}
