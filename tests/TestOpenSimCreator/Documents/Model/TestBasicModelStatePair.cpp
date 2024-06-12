#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <Simbody.h>

#include <filesystem>

using namespace osc;

TEST(BasicModelStatePair, WhenConstructedFromFilepathLoadsTheOsimFileAndInitializesIt)
{
    std::filesystem::path const modelPath = std::filesystem::path{OSC_RESOURCES_DIR} / "models" / "Arm26" / "arm26.osim";

    BasicModelStatePair p{modelPath};
    ASSERT_GE(p.getState().getSystemStage(), SimTK::Stage::Dynamics);
}
