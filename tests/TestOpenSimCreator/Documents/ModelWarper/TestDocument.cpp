#include <OpenSimCreator/Documents/ModelWarper/Document.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.hpp>
#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <cctype>
#include <filesystem>
#include <stdexcept>

using osc::mow::Document;
using osc::mow::MeshWarpPairing;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(ModelWarpingDocument, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ Document{}; });
}

TEST(ModelWarpingDocument, CanConstructFromPathToOsim)
{
    ASSERT_NO_THROW({ Document{GetFixturesDir() / "blank.osim"}; });
}

TEST(ModelWarpingDocument, ConstructorThrowsIfGivenInvalidOsimPath)
{
    ASSERT_THROW({ Document{std::filesystem::path{"bs.osim"}}; }, std::exception);
}

TEST(ModelWarpingDocument, AfterConstructingFromBasicOsimFileTheReturnedModelContainsExpectedComponents)
{
    Document const doc{GetFixturesDir() / "onebody.osim"};
    doc.getModel().getComponent("bodyset/some_body");
}

TEST(ModelWarpingDocument, HasFrameDefinitionFileReturnsFalseForSimpleNonFramedCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "Simple" / "model.osim";
    ASSERT_FALSE(Document{modelPath}.hasFrameDefinitionFile());
}

TEST(ModelWarpingDocument, HasFrameDefinitionFileReturnsTrueForSimpleFramedCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "SimpleFramed" / "model.osim";
    ASSERT_TRUE(Document{modelPath}.hasFrameDefinitionFile());
}


TEST(ModelWarpingDocument, RecommendedFrameDefinitionFilepathWorksAsExpected)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "SimpleFramed" / "model.osim";
    std::filesystem::path const expectedFramesPath = GetFixturesDir() / "SimpleFramed" / "model.frames.toml";
    ASSERT_EQ(Document{modelPath}.recommendedFrameDefinitionFilepath(), expectedFramesPath);
}

TEST(ModelWarpingDocument, HasFramesFileLoadErrorReturnsFalseForNormalCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "SimpleFramed" / "model.osim";
    ASSERT_FALSE(Document{modelPath}.hasFramesFileLoadError());
}

TEST(ModelWarpingDocument, HasFramesFileLoadErrorReturnsTrueForBadCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "BadFramesFile" / "model.osim";
    ASSERT_TRUE(Document{modelPath}.hasFramesFileLoadError());
}

TEST(ModelWarpingDocument, GetFramesFileLoadErrorReturnsNulloptForNormalCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "SimpleFramed" / "model.osim";
    ASSERT_EQ(Document{modelPath}.getFramesFileLoadError(), std::nullopt);
}

TEST(ModelWarpingDocument, GetFramesFileLoadErrorReturnsNonNulloptForBadCase)
{
    std::filesystem::path const modelPath = GetFixturesDir() / "BadFramesFile" / "model.osim";
    ASSERT_NE(Document{modelPath}.getFramesFileLoadError(), std::nullopt);
}
