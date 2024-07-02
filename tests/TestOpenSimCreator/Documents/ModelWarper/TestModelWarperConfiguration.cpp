#include <OpenSimCreator/Documents/ModelWarper/ModelWarperConfiguration.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
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

TEST(ModelWarperConfiguration, CanDefaultConstruct)
{
    [[maybe_unused]] ModelWarperConfiguration configuration;
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
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});
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
