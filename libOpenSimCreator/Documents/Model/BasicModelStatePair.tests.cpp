#include "BasicModelStatePair.h"

#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>
#include <libopensimcreator/testing/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <Simbody.h>

#include <filesystem>

using namespace osc;

TEST(BasicModelStatePair, WhenConstructedFromFilepathLoadsTheOsimFileAndInitializesIt)
{
    const std::filesystem::path modelPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePair p{modelPath};
    ASSERT_GE(p.getState().getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePair, HasAFullyRealizedStateWhenCopied)
{
    BasicModelStatePair p;
    ASSERT_EQ(p.getState().getSystemStage(), SimTK::Stage::Dynamics);
    const BasicModelStatePair copy{p};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(copy.getState(). getSystemStage(), SimTK::Stage::Dynamics);
}

TEST(BasicModelStatePair, CanGenerateDecorationsFromCopy)
{
    GloballyInitOpenSim();
    GloballyAddDirectoryToOpenSimGeometrySearchPath(std::filesystem::path{OSC_RESOURCES_DIR} / "geometry");

    const std::filesystem::path modelPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Arm26/arm26.osim";

    BasicModelStatePair p{modelPath};
    SceneCache cache;
    ASSERT_NO_THROW({ GenerateModelDecorations(cache, p); });
    const BasicModelStatePair copy{p};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_NO_THROW({ GenerateModelDecorations(cache, copy); });
}
