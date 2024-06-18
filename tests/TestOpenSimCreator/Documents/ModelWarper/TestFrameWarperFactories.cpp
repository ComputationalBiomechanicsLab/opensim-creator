#include <OpenSimCreator/Documents/ModelWarper/FrameWarperFactories.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSimCreator/Documents/ModelWarper/IdentityFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/StationDefinedFrameWarperFactory.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <filesystem>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::filesystem::path ModelWarperFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(FrameWarperFactories, CanDefaultConstruct)
{
    // sanity check: this ensures it's semiregular and, therefore, can be composed into larger
    // datastructures without too many tears

    ASSERT_NO_THROW({ FrameWarperFactories{}; });
}

TEST(FrameWarperFactories, WhenLoadingModelContainingPofsButNoWarpingConfigDoesNotPopulateWarpFactoriesForPofs)
{
    // tests that if a an `.osim` is loaded that contains `OpenSim::PhysicalOffsetFrame`s (PoFs),
    // but the `.osim` has no associated warping configuration, then the lookup should leave
    // the frame's warper as "unpopulated", meaning "I don't know what to do with this"
    //
    // if the user wants to ignore a frame, they should explicitly specify it in the model's
    // warp configuration (either globally, as in "identity-warp all PoFs", or locally, as in
    // "identity-warp this PoF specifically")

    const auto osimFileLocation = ModelWarperFixturesDir() / "PofPaired" / "model.osim";
    const OpenSim::Model model{osimFileLocation.string()};
    const ModelWarpConfiguration config{osimFileLocation, model};

    ASSERT_TRUE(FrameWarperFactories(osimFileLocation, model, config).empty());
}

TEST(FrameWarperFactories, WhenLoadingModelContainingPofsAndDefaultedWarpingPopulatesWarpFactoriesWithIdentityWarps)
{
    // tests that if an `.osim` is loaded that contains `OpenSim::PhysicalOffsetFrame`s (PoFs), and there
    // is also an associated warping configuration that says "identity warp missing data", then the lookup
    // should give identity warps to the PoFs

    const auto osimFileLocation = ModelWarperFixturesDir() / "PofPairedIdentityWarp" / "model.osim";
    OpenSim::Model model{osimFileLocation.string()};
    const ModelWarpConfiguration config{osimFileLocation, model};

    InitializeModel(model);
    InitializeState(model);

    FrameWarperFactories lookup(osimFileLocation, model, config);

    ASSERT_FALSE(lookup.empty()) << "should populate lookup with identity warps (as specified in the config)";
    for (const auto& pof : model.getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        if (const IFrameWarperFactory* factory =  lookup.find(GetAbsolutePathString(pof))) {
            ASSERT_TRUE(dynamic_cast<const IdentityFrameWarperFactory*>(factory)) << "every PoF should have an identity warp";
        }
    }
}

TEST(FrameWarperFactories, WhenLoadingAModelUsingStationDefinedFramesAssignsStationDefinedFrameWarperToTheFrames)
{
    // tests that if an `.osim` is loaded that exclusively uses `OpenSim::StationDefinedFrame`s, then the lookup
    // is populated with `StationDefinedFrameWarperFactory`s, rather than `IdentityFrameWarperFactory`s, because
    // the implementation knows that these are safe frames to warp (so the user need not override things, etc.)

    const auto osimFileLocation = ModelWarperFixturesDir() / "StationDefinedFramePaired" / "model.osim";
    OpenSim::Model model{osimFileLocation.string()};
    const ModelWarpConfiguration config{osimFileLocation, model};  // note: it has no associated config file

    InitializeModel(model);
    InitializeState(model);

    FrameWarperFactories lookup(osimFileLocation, model, config);
    ASSERT_FALSE(lookup.empty()) << "should populate lookup with station defined frame warps (even without a config: this is default behavior)";
    for (const auto& pof : model.getComponentList<OpenSim::StationDefinedFrame>()) {
        if (const IFrameWarperFactory* factory =  lookup.find(GetAbsolutePathString(pof))) {
            ASSERT_TRUE(dynamic_cast<const StationDefinedFrameWarperFactory*>(factory)) << "every SdF should have a StationDefinedFrame warp";
        }
    }
}
