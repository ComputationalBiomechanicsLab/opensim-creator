#include "basic_model_state_pair_with_shared_environment.h"

#include <libopensimcreator/platform/open_sim_creator_app.h>
#include <libopensimcreator/tests/testopensimcreatorconfig.h>

#include <gtest/gtest.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <SimTKcommon/internal/State.h>

#include <filesystem>

using namespace osc;

TEST(BasicModelStatePairWithSharedEnvironment, WhenConstructedFromFilepathLoadsTheOsimFileAndInitializesIt)
{
    const std::filesystem::path modelPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePairWithSharedEnvironment p{modelPath};
    ASSERT_GE(p.getState().getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePairWithSharedEnvironment, HasAFullyRealizedStateWhenCopied)
{
    BasicModelStatePairWithSharedEnvironment p;
    ASSERT_EQ(p.getState().getSystemStage(), SimTK::Stage::Dynamics);
    const BasicModelStatePairWithSharedEnvironment copy{p};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(copy.getState(). getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePairWithSharedEnvironment, CanGenerateDecorationsFromCopy)
{
    GloballyInitOpenSim();
    GloballyAddDirectoryToOpenSimGeometrySearchPath(std::filesystem::path{OSC_RESOURCES_DIR} / "geometry");

    const std::filesystem::path modelPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePairWithSharedEnvironment p{modelPath};
    SceneCache cache;
    ASSERT_NO_THROW({ opyn::GenerateModelDecorations(cache, p); });
    const BasicModelStatePairWithSharedEnvironment copy{p};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_NO_THROW({ opyn::GenerateModelDecorations(cache, copy); });
}
