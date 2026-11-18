#include "basic_model_state_pair_with_shared_environment.h"

#include <libopensimcreator/platform/open_sim_creator_app.h>
#include <libopensimcreator/tests/testopensimcreatorconfig.h>

#include <gtest/gtest.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <SimTKcommon/internal/State.h>

#include <filesystem>

using namespace osc;

TEST(
    BasicModelStatePairWithSharedEnvironment,
    WhenConstructedFromFilepathLoadsTheOsimFileAndInitializesIt)
{
    const std::filesystem::path model_path =
        std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePairWithSharedEnvironment p{model_path};
    ASSERT_GE(p.get_state().getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePairWithSharedEnvironment, HasAFullyRealizedStateWhenCopied)
{
    BasicModelStatePairWithSharedEnvironment p;
    ASSERT_EQ(p.get_state().getSystemStage(), SimTK::Stage::Dynamics);
    const BasicModelStatePairWithSharedEnvironment copy{p};
    ASSERT_EQ(copy.get_state().getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePairWithSharedEnvironment, CanGenerateDecorationsFromCopy)
{
    globally_init_open_sim();
    globally_add_directory_to_open_sim_geometry_search_path(
        std::filesystem::path{OSC_RESOURCES_DIR} / "geometry");

    const std::filesystem::path model_path =
        std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePairWithSharedEnvironment p{model_path};
    SceneCache cache;
    ASSERT_NO_THROW({ opyn::generate_model_decorations(cache, p); });
    const BasicModelStatePairWithSharedEnvironment copy{p};
    ASSERT_NO_THROW({ opyn::generate_model_decorations(cache, copy); });
}
