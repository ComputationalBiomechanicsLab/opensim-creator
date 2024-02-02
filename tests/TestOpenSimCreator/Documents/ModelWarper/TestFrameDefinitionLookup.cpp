#include <OpenSimCreator/Documents/ModelWarper/FrameDefinitionLookup.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <OpenSim/Simulation/Model/Model.h>
#include <gtest/gtest.h>

#include <filesystem>

using osc::mow::FrameDefinitionLookup;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(FrameDefinitionLookup, FindFrameDefinitionReturnsNullptrForNonExistentFrame)
{
    std::filesystem::path const path = GetFixturesDir() / "SimpleFramed" / "model.osim";
    OpenSim::Model const model{path.string()};
    FrameDefinitionLookup const lut{path, model};

    ASSERT_EQ(lut.lookup("some-nonexistent-framedef"), nullptr);
}

TEST(FrameDefinitionLookup, FindFrameDefinitionReturnsNonNullptrForExistentFrame)
{
    std::filesystem::path const path = GetFixturesDir() / "SimpleFramed" / "model.osim";
    OpenSim::Model const model{path.string()};
    FrameDefinitionLookup const lut{path, model};

    ASSERT_NE(lut.lookup("/jointset/weldjoint/ground_offset"), nullptr);
}
