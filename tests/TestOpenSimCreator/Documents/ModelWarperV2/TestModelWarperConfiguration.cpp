#include <OpenSimCreator/Documents/ModelWarperV2/ModelWarperConfiguration.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Utils/LandmarkPair3D.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/TemporaryFile.h>

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <ranges>

using namespace osc;
using namespace osc::mow;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path GetFixturePath(const std::filesystem::path& subpath)
    {
        return std::filesystem::weakly_canonical(std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / subpath);
    }

    // A testing class that can act as an inner node in a model's component tree.
    class ContainerNode : public OpenSim::Component {
        OpenSim_DECLARE_CONCRETE_OBJECT(ContainerNode, OpenSim::Component)
    };
}

TEST(StrategyMatchQuality, NoneComparesLessThanWildcard)
{
    // A wildcard match is "better than" no match.
    static_assert(StrategyMatchQuality::none() < StrategyMatchQuality::wildcard());
}

TEST(StrategyMatchQuality, WildcardComparesLessThanExact)
{
    // An exact match is "better than" a wildcard match.
    static_assert(StrategyMatchQuality::wildcard() < StrategyMatchQuality::exact());
}

TEST(StrategyMatchQuality, NoneImplicitlyConvertsToFalse)
{
    // The truthiness of a `StrategyMatchQuality` corresponds to "is there any match?".
    static_assert(not static_cast<bool>(StrategyMatchQuality::none()));
}

TEST(StrategyMatchQuality, WildcardImplicitlyConvertsToTrue)
{
    // The truthiness of a `StrategyMatchQuality` corresponds to "is there any match?".
    static_assert(static_cast<bool>(StrategyMatchQuality::wildcard()));
}

TEST(StrategyMatchQuality, ExactImplicitlyConvertsToTrue)
{
    // The truthiness of a `StrategyMatchQuality` corresponds to "is there any match?".
    static_assert(static_cast<bool>(StrategyMatchQuality::exact()));
}

TEST(RuntimeWarpParameters, ConstructedWithBlendFactorMakesGetBlendFactorReturnTheBlendFactor)
{
    const RuntimeWarpParameters params{0.3f};
    ASSERT_EQ(params.getBlendFactor(), 0.3f);
}

TEST(WarpCache, CanDefaultConstruct)
{
    [[maybe_unused]] const WarpCache instance;
}

TEST(IdentityComponentWarper, CanDefaultConstruct)
{
    [[maybe_unused]] const IdentityComponentWarper instance;
}

TEST(IdentityComponentWarper, DoesNotChangeAnyComponentProperty)
{
    // An `IdentityComponentWarper` shouldn't do anything to a component when it "warps" it.

    OpenSim::Model sourceModel;
    OpenSim::Marker& sourceMarker = AddMarker(sourceModel, "marker", sourceModel.getGround(), SimTK::Vec3{0.0});
    FinalizeConnections(sourceModel);
    InitializeModel(sourceModel);
    OpenSim::Model destinationModel = sourceModel;  // create copy for writing
    InitializeModel(destinationModel);
    auto& destinationMarker = sourceModel.updComponent<OpenSim::Marker>(sourceMarker.getAbsolutePath());

    const RuntimeWarpParameters parameters;
    WarpCache cache;
    IdentityComponentWarper warper;

    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
    warper.warpInPlace(parameters, cache, sourceModel, sourceMarker, destinationModel, destinationMarker);
    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
}

TEST(ExceptionThrowingComponentWarper, CanDefaultConstruct)
{
    [[maybe_unused]] const ExceptionThrowingComponentWarper instance;
}

TEST(ExceptionThrowingComponentWarper, ThrowsWhenWarpInPlaceIsCalled)
{
    // An `ExceptionThrowingComponentWarper` is specifically designed to throw an exception when
    // it tries to warp a component (this behavior can be useful as a catch-all).

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
    }
    catch (const std::exception& ex) {
        ASSERT_TRUE(contains(ex.what(), "some message content"));
        warperThrownException = true;
    }
    ASSERT_TRUE(warperThrownException) << "should always throw an exception";
    ASSERT_TRUE(destinationMarker.isObjectUpToDateWithProperties());
}

TEST(PairedPoints, CanDefaultConstruct)
{
    [[maybe_unused]] PairedPoints instance;
}

TEST(PairedPoints, CanConstructFromRangeOfPairedPointsPlusBaseOffsetPath)
{
    const auto points = std::to_array<LandmarkPair3D>({
        {Vec3{0.0f}, Vec3{1.0f}},
        {Vec3{2.0f}, Vec3{3.0f}},
    });
    const OpenSim::ComponentPath path{"/bodyset/somebody"};

    const PairedPoints pps{points, path};

    ASSERT_EQ(pps.getBaseFrameAbsPath(), path);
    ASSERT_TRUE(rgs::equal(pps, points));
}

TEST(PairedPoints, CopyingPointsWorksAsExpected)
{
    const auto points = std::to_array<LandmarkPair3D>({
        {Vec3{0.0f}, Vec3{1.0f}},
        {Vec3{2.0f}, Vec3{3.0f}},
    });
    const OpenSim::ComponentPath path{"/bodyset/somebody"};

    const PairedPoints pps{points, path};
    const PairedPoints copy = pps;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(pps.getBaseFrameAbsPath(), copy.getBaseFrameAbsPath());
    ASSERT_TRUE(rgs::equal(pps, copy));
}

TEST(PairedPoints, CopyComparesEqualToOriginal)
{
    const auto points = std::to_array<LandmarkPair3D>({
        {Vec3{0.0f}, Vec3{1.0f}},
        {Vec3{2.0f}, Vec3{3.0f}},
    });
    const OpenSim::ComponentPath path{"/bodyset/somebody"};

    const PairedPoints pps{points, path};
    const PairedPoints copy = pps;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(pps, copy);
}

TEST(PairedPoints, EqualityIsValueBased)
{
    const auto points = std::to_array<LandmarkPair3D>({
        {Vec3{0.0f}, Vec3{1.0f}},
        {Vec3{2.0f}, Vec3{3.0f}},
    });
    const OpenSim::ComponentPath path{"/bodyset/somebody"};

    // construct two independent instances (no copying)
    const PairedPoints a{points, path};
    const PairedPoints b{points, path};

    ASSERT_EQ(a, b);
}

namespace
{
    // A testing class for testing the `PairedPointSource`-related APIs.
    class TestablePairedPointSource final : public PairedPointSource {
        OpenSim_DECLARE_CONCRETE_OBJECT(TestablePairedPointSource, PairedPointSource)
    public:
        template<std::ranges::input_range Range>
        requires std::convertible_to<std::ranges::range_value_t<Range>, ValidationCheckResult>
        void setChecks(Range&& checks)
        {
            m_Checks.assign(rgs::begin(checks), rgs::end(checks));
        }

        void setPairedPoints(const PairedPoints& points)
        {
            m_Points = points;
        }
    private:
        PairedPoints implGetPairedPoints(
            WarpCache&,
            const OpenSim::Model&,
            const OpenSim::Component&) final
        {
            return m_Points;
        }

        std::vector<ValidationCheckResult> implValidate(
            const OpenSim::Model&,
            const OpenSim::Component&) const final
        {
            return m_Checks;
        }

        PairedPoints m_Points;
        std::vector<ValidationCheckResult> m_Checks;
    };
}

TEST(PairedPointSource, getPairedPoints_returnsPairedPoints)
{
    const PairedPoints points{
        std::to_array<LandmarkPair3D>({
            {Vec3{}, Vec3{}},
            {Vec3{}, Vec3{}},
        }),
        OpenSim::ComponentPath{"somebaseframe"},
    };

    TestablePairedPointSource mock;
    mock.setPairedPoints(points);

    WarpCache cache;
    const OpenSim::Model sourceModel;
    const auto& sourceComponent = sourceModel.getGround();
    const PairedPoints returnedPoints = mock.getPairedPoints(cache, sourceModel, sourceComponent);

    ASSERT_EQ(returnedPoints, points);
}

TEST(PairedPointSource, getPairedPoints_validateReturnsValidationChecks)
{
    const std::vector<ValidationCheckResult> checks = {
        {"some ok check", ValidationCheckState::Ok},
        {"some warning check", ValidationCheckState::Warning},
        {"some error check", ValidationCheckState::Error},
    };

    TestablePairedPointSource mock;
    mock.setChecks(checks);

    const OpenSim::Model sourceModel;
    const auto& sourceComponent = sourceModel.getGround();
    const std::vector<ValidationCheckResult> returnedChecks = mock.validate(sourceModel, sourceComponent);

    ASSERT_EQ(returnedChecks, checks);
}

TEST(PairedPointSource, getPairedPoints_throwsIfValidationChecksContainError)
{
    const std::vector<ValidationCheckResult> checks = {
        {"uh oh", ValidationCheckState::Error},
    };

    TestablePairedPointSource mock;
    mock.setChecks(checks);

    WarpCache cache;
    const OpenSim::Model sourceModel;
    const auto& sourceComponent = sourceModel.getGround();
    ASSERT_THROW({ mock.getPairedPoints(cache, sourceModel, sourceComponent); }, std::exception);
}

TEST(PairedPointSource, getPairedPoints_doesntThrowIfChecksContainWarning)
{
    const std::vector<ValidationCheckResult> checks = {
        {"should be ok", ValidationCheckState::Warning},
    };

    TestablePairedPointSource mock;
    mock.setChecks(checks);

    WarpCache cache;
    const OpenSim::Model sourceModel;
    const auto& sourceComponent = sourceModel.getGround();
    ASSERT_NO_THROW({ mock.getPairedPoints(cache, sourceModel, sourceComponent); });
}

TEST(LandmarkPairsAssociatedWithMesh, CanBeDefaultConstructed)
{
    [[maybe_unused]] const LandmarkPairsAssociatedWithMesh instance;
}

TEST(LandmarkPairsAssociatedWithMesh, ValidateReturnsErrorIfProvidedNonMesh)
{
    const LandmarkPairsAssociatedWithMesh pairSource;
    const OpenSim::Model model;
    const auto checks = pairSource.validate(model, model.getGround());

    ASSERT_TRUE(rgs::any_of(checks, &ValidationCheckResult::is_error));
}

TEST(LandmarkPairsAssociatedWithMesh, ValidateReturnsErrorIfProvidedMeshWithoutSourceLandmarksButWithDestinationLandmarks)
{
    // Note: `sphere.obj` doesn't have an associated source `sphere.landmarks.csv` file.
    const std::filesystem::path sourceMeshPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Document/ModelWarper/MissingSourceLMs/Geometry/sphere.obj";

    // Create an `OpenSim::Model` that contains the mesh.
    OpenSim::Model model;
    auto& mesh = AddComponent<OpenSim::Mesh>(model, sourceMeshPath.string());
    mesh.connectSocket_frame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);

    const LandmarkPairsAssociatedWithMesh pointSource;
    const auto checks = pointSource.validate(model, mesh);

    ASSERT_TRUE(rgs::any_of(checks, &ValidationCheckResult::is_error));
}

TEST(LandmarkPairsAssociatedWithMesh, ValidateReturnsErrorIfProvidedMeshWithSourceLandmarksButNoDestinationLandmarks)
{
    // Note: `sphere.obj` doesn't have an associated destination `sphere.landmarks.csv` file.
    const std::filesystem::path sourceMeshPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Document/ModelWarper/MissingDestinationLMs/Geometry/sphere.obj";

    // Create an `OpenSim::Model` that contains the mesh.
    OpenSim::Model model;
    auto& mesh = AddComponent<OpenSim::Mesh>(model, sourceMeshPath.string());
    mesh.connectSocket_frame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);

    const LandmarkPairsAssociatedWithMesh pointSource;
    const auto checks = pointSource.validate(model, mesh);

    ASSERT_TRUE(rgs::any_of(checks, &ValidationCheckResult::is_error));
}

TEST(ModelWarperConfiguration, CanDefaultConstruct)
{
    [[maybe_unused]] const ModelWarperConfiguration instance;
}

TEST(ModelWarperConfiguration, CanSaveAndLoadDefaultConstructedToAndFromXMLFile)
{
    TemporaryFile temporaryFile;
    temporaryFile.close();  // so that `OpenSim::Object::print`'s implementation can open+write to it

    // Write a blank `ModelWarperConfiguration` file.
    const ModelWarperConfiguration configuration;
    configuration.print(temporaryFile.absolute_path().string());

    // Read the written file.
    ModelWarperConfiguration loadedConfiguration{temporaryFile.absolute_path().string()};
    loadedConfiguration.finalizeFromProperties();
    loadedConfiguration.finalizeConnections(loadedConfiguration);
}

TEST(ModelWarperConfiguration, LoadingNonExistentFileThrowsAnException)
{
    ASSERT_ANY_THROW({ [[maybe_unused]] const ModelWarperConfiguration configuration{GetFixturePath("doesnt_exist")}; });
}

TEST(ModelWarperConfiguration, CanLoadEmptySequence)
{
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/empty_sequence.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleOffsetFrameWarpingStrategy)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});

    // Load a `ModelWarperConfiguration` with a single `ProduceErrorOffsetFrameWarpingStrategy` that
    // wildcard-matches all `PhysicalOffsetFrame`s in the model.
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/single_offsetframe_warper.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    // Confirm the strategy was loaded
    {
        auto range = configuration.getComponentList<ProduceErrorOffsetFrameWarpingStrategy>();
        const auto numEls = std::distance(range.begin(), range.end());
        ASSERT_EQ(numEls, 1);
    }

    // Create a model with a `PhysicalOffsetFrame`.
    OpenSim::Model model;
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{});
    model.finalizeConnections();

    // Enusre it matches.
    const ComponentWarpingStrategy* strategy = configuration.tryMatchStrategy(pof);
    ASSERT_TRUE(strategy);
    ASSERT_EQ(strategy->getName(), "warp1");
    ASSERT_TRUE(dynamic_cast<const ProduceErrorOffsetFrameWarpingStrategy*>(strategy));
}

TEST(ModelWarperConfiguration, CanContainAMixtureOfOffsetFrameWarpingStrategies)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/mixed_offsetframe_warpers.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    auto range = configuration.getComponentList<OffsetFrameWarpingStrategy>();
    const auto numEls = std::distance(range.begin(), range.end());
    ASSERT_EQ(numEls, 2);
}

TEST(ModelWarperConfiguration, PrefersMoreSpecificOffsetFrameWarperIfAvailable)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy{});

    // Load a `ModelWarperConfiguration` that has two offset frame warping strategies with
    // differing `StrategyTargets` specificity.
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/mixed_offsetframe_warpers.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    // Make an `OpenSim::Model` that contains two frames: one that perfectly matches the
    // specific `StrategyTarget` and another one that doesn't.
    OpenSim::Model model;
    auto& something = AddComponent<ContainerNode>(model);
    something.setName("something");
    auto& more = AddComponent<ContainerNode>(something);
    more.setName("more");
    auto& specific = AddComponent<OpenSim::PhysicalOffsetFrame>(more, model.getGround(), SimTK::Transform{});
    specific.setName("specific");
    auto& topLevel = AddComponent<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{});
    model.finalizeConnections();

    // Ensure that the specific strategy is matched with the specific component.
    const ComponentWarpingStrategy* specificMatch = configuration.tryMatchStrategy(specific);
    ASSERT_TRUE(specificMatch);
    ASSERT_EQ(specificMatch->getName(), "warp2");

    // Ensure that the wildcard fallback strategy is matched with the other component.
    const ComponentWarpingStrategy* topLevelMatch = configuration.tryMatchStrategy(topLevel);
    ASSERT_TRUE(topLevelMatch);
    ASSERT_EQ(topLevelMatch->getName(), "warp1");
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleStationWarpingStrategy)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    // Load a `ModelWarperConfiguration` with a single `ProduceErrorStationWarpingStrategy`
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/single_station_warper.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    // Confirm the strategy was loaded.
    {
        auto range = configuration.getComponentList<ProduceErrorStationWarpingStrategy>();
        const auto numEls = std::distance(range.begin(), range.end());
        ASSERT_EQ(numEls, 1);
    }

    // Build an `OpenSim::Model` that contains an `OpenSim::Station`.
    OpenSim::Model model;
    auto& station = AddComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{0.0});
    model.finalizeConnections();

    // Ensure the strategy matches against the station.
    const ComponentWarpingStrategy* strategy = configuration.tryMatchStrategy(station);
    ASSERT_TRUE(strategy);
    ASSERT_EQ(strategy->getName(), "warp1");
    ASSERT_TRUE(dynamic_cast<const ProduceErrorStationWarpingStrategy*>(strategy));
}

TEST(ModelWarperConfiguration, CanLoadAMixtureOfStationWarpingStrategies)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});
    OpenSim::Object::registerType(ThinPlateSplineStationWarpingStrategy{});

    // Load a `ModelWarperConfiguration` containing two `StationWarpingStrategy`s: a more specific
    // one, and a wildcard one.
    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/mixed_station_warpers.xml")};
    configuration.finalizeFromProperties();
    configuration.finalizeConnections(configuration);

    // Confirm that two strategies were loaded.
    {
        auto range = configuration.getComponentList<StationWarpingStrategy>();
        const auto numEls = std::distance(range.begin(), range.end());
        ASSERT_EQ(numEls, 2);
    }

    // Create an `OpenSim::Model` containing two `Open::Station`s: one that's located
    // at the specific path and another somewhere else.
    OpenSim::Model model;
    auto& something = AddComponent<ContainerNode>(model);
    something.setName("something");
    auto& more = AddComponent<ContainerNode>(something);
    more.setName("more");
    auto& specific = AddComponent<OpenSim::Station>(more, model.getGround(), SimTK::Vec3{0.0});
    specific.setName("specific");
    auto& general = AddComponent<OpenSim::Station>(something, model.getGround(), SimTK::Vec3{0.0});
    general.setName("doesntmatter");

    // Ensure that the specific strategy matches to the specific `OpenSim::Station`.
    {
        const ComponentWarpingStrategy* strategy = configuration.tryMatchStrategy(specific);
        ASSERT_TRUE(strategy);
        ASSERT_EQ(strategy->getName(), "warp2");
        ASSERT_TRUE(dynamic_cast<const ThinPlateSplineStationWarpingStrategy*>(strategy));
    }

    // Ensure that the wildcard strategy matches to the other `OpenSim::Station`.
    {
        const ComponentWarpingStrategy* strategy = configuration.tryMatchStrategy(general);
        ASSERT_TRUE(strategy);
        ASSERT_EQ(strategy->getName(), "warp1");
        ASSERT_TRUE(dynamic_cast<const ProduceErrorStationWarpingStrategy*>(strategy));
    }
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesFailsIfNoStrategyTargets)
{
    ProduceErrorOffsetFrameWarpingStrategy strategy;
    ASSERT_ANY_THROW({ strategy.finalizeFromProperties(); }) << "should fail, because the strategy has no targets (ambiguous definition)";
}

TEST(ProduceErrorOffsetFrameWarpingStrategy, FinalizeFromPropertiesWorksIfThereIsAtLeastOneStrategyTarget)
{
    ProduceErrorOffsetFrameWarpingStrategy strategy;
    strategy.append_StrategyTargets("*");
    ASSERT_NO_THROW({ strategy.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, LoadingConfigurationContainingStrategyWithTwoTargetsWorksAsExpected)
{
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/two_strategy_targets.xml")};
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

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/duplicated_offsetframe_strategytarget.xml")};

    ASSERT_ANY_THROW({ configuration.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, finalizeFromPropertiesDoesNotThrowWhenGivenConfigurationContainingTwoDifferentTypesOfStrategiesWithTheSameStrategyTarget)
{
    OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
    OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

    ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarperV2/duplicated_but_different_types.xml")};

    ASSERT_NO_THROW({ configuration.finalizeFromProperties(); });
}

TEST(ModelWarperConfiguration, MatchingAnOffsetFrameStrategyToExactPathWorksAsExpected)
{
    OpenSim::Model model;
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
    auto& pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
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
