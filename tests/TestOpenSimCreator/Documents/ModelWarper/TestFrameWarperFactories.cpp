#include <OpenSimCreator/Documents/ModelWarper/FrameWarperFactories.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>

#include <filesystem>

using namespace osc::mow;

namespace
{
    std::filesystem::path ModelWarperFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/ModelWarper";
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

    auto const osimFileLocation = ModelWarperFixturesDir() / "PofPaired" / "model.osim";
    OpenSim::Model const model{osimFileLocation.string()};
    ModelWarpConfiguration const config{osimFileLocation, model};

    ASSERT_TRUE(FrameWarperFactories(osimFileLocation, model, config).empty());
}
