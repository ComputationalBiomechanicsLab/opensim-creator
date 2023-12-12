#include <OpenSimCreator/Documents/Frames/FramesHelpers.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <gtest/gtest.h>
#include <OpenSimCreator/Documents/Frames/FrameAxis.hpp>
#include <OpenSimCreator/Documents/Frames/FrameDefinition.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using osc::frames::FrameAxis;
using osc::frames::FrameDefinition;
using osc::frames::FramesFile;
using osc::frames::ReadFramesFromTOML;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/Frames";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }

    std::ifstream OpenFixtureFile(std::string_view fixtureName)
    {
        std::filesystem::path const path = GetFixturesDir() / fixtureName;

        std::ifstream f{path};
        if (!f)
        {
            std::stringstream ss;
            ss << path << ": cannot open fixture path";
            throw std::runtime_error{std::move(ss).str()};
        }
        return f;
    }
}

TEST(FramesHelpers, ReadFramesFromTOMLCanReadBlankFixture)
{
    std::ifstream f = OpenFixtureFile("empty.frames.toml");
    FramesFile const parsed = ReadFramesFromTOML(f);
    ASSERT_FALSE(parsed.hasFrameDefinitions());
}

TEST(FramesHelpers, ReadFramesFromTOMLCorrectlyReadsBasicFile)
{
    std::ifstream f = OpenFixtureFile("basic.frames.toml");
    FramesFile const parsed = ReadFramesFromTOML(f);

    ASSERT_TRUE(parsed.hasFrameDefinitions());
    ASSERT_EQ(parsed.getNumFrameDefinitions(), 1);

    FrameDefinition const& def = parsed.getFrameDefinition(0);
    ASSERT_EQ(def.getName(), "first");
    ASSERT_EQ(def.getAssociatedMeshName(), "sphere.obj");
    ASSERT_EQ(def.getOriginLocationLandmarkName(), "LM2");
    ASSERT_EQ(def.getAxisEdgeBeginLandmarkName(), "LM2");
    ASSERT_EQ(def.getAxisEdgeEndLandmarkName(), "LM3");
    ASSERT_EQ(def.getAxisEdgeAxis(), FrameAxis::MinusX);
    ASSERT_EQ(def.getNonParallelEdgeBeginLandmarkName(), "some_other_lm");
    ASSERT_EQ(def.getNonParallelEdgeEndLandmarkName(), "another_one");
    ASSERT_EQ(def.getCrossProductEdgeAxis(), FrameAxis::PlusY);
}

TEST(FramesHelpers, ReadFramesFromTOMLCorrectlyReadsTwoFrameFile)
{
    std::ifstream f = OpenFixtureFile("two.frames.toml");
    FramesFile const parsed = ReadFramesFromTOML(f);

    ASSERT_TRUE(parsed.hasFrameDefinitions());
    ASSERT_EQ(parsed.getNumFrameDefinitions(), 2);

    // check first
    {
        FrameDefinition const& def = parsed.getFrameDefinition(0);
        ASSERT_EQ(def.getName(), "first");
        ASSERT_EQ(def.getAssociatedMeshName(), "sphere.obj");
        ASSERT_EQ(def.getOriginLocationLandmarkName(), "LM1");
        ASSERT_EQ(def.getAxisEdgeBeginLandmarkName(), "LM2");
        ASSERT_EQ(def.getAxisEdgeEndLandmarkName(), "LM3");
        ASSERT_EQ(def.getAxisEdgeAxis(), FrameAxis::MinusX);
        ASSERT_EQ(def.getNonParallelEdgeBeginLandmarkName(), "LM4");
        ASSERT_EQ(def.getNonParallelEdgeEndLandmarkName(), "LM5");
        ASSERT_EQ(def.getCrossProductEdgeAxis(), FrameAxis::MinusY);
    }

    // check second
    {
        FrameDefinition const& def = parsed.getFrameDefinition(1);
        ASSERT_EQ(def.getName(), "second");
        ASSERT_EQ(def.getAssociatedMeshName(), "cylinder.obj");
        ASSERT_EQ(def.getOriginLocationLandmarkName(), "LM5");
        ASSERT_EQ(def.getAxisEdgeBeginLandmarkName(), "LM6");
        ASSERT_EQ(def.getAxisEdgeEndLandmarkName(), "LM7");
        ASSERT_EQ(def.getAxisEdgeAxis(), FrameAxis::PlusZ);
        ASSERT_EQ(def.getNonParallelEdgeBeginLandmarkName(), "LM8");
        ASSERT_EQ(def.getNonParallelEdgeEndLandmarkName(), "LM9");
        ASSERT_EQ(def.getCrossProductEdgeAxis(), FrameAxis::MinusX);
    }
}

TEST(FramesHelpers, ReadFramesFromTOMLThrowsIfGivenInvalidTOML)
{
    std::ifstream f = OpenFixtureFile("invalid.frames.toml");
    ASSERT_ANY_THROW({ ReadFramesFromTOML(f); });
}

TEST(FramesHelpers, ThrowsIfMissingNecessaryData)
{
    std::ifstream f = OpenFixtureFile("missing_fields.frames.toml");
    ASSERT_ANY_THROW({ ReadFramesFromTOML(f); });
}

TEST(FramesHelpers, ThrowsIfGivenNonOrthogonalFrames)
{
    std::ifstream f = OpenFixtureFile("not_orthogonal.frames.toml");
    ASSERT_ANY_THROW({ ReadFramesFromTOML(f); });
}

TEST(FramesHelpers, ThrowsIfGivenNonEdgeAxis)
{
    std::ifstream f = OpenFixtureFile("not_edge_axis.frames.toml");
    ASSERT_ANY_THROW({ ReadFramesFromTOML(f); });
}

TEST(FramesHelpers, ThrowsIfGivenNonEdgeNonparallelAxis)
{
    std::ifstream f = OpenFixtureFile("not_edge_nonparallel.frames.toml");
    ASSERT_ANY_THROW({ ReadFramesFromTOML(f); });
}
