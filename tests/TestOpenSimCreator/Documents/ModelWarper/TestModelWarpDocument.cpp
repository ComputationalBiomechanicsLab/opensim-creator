#include <OpenSimCreator/Documents/ModelWarper/ModelWarpDocument.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Maths/Constants.h>

#include <cctype>
#include <filesystem>
#include <stdexcept>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(ModelWarpDocument, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ ModelWarpDocument{}; });
}

TEST(ModelWarpDocument, CanConstructFromPathToOsim)
{
    ASSERT_NO_THROW({ ModelWarpDocument{GetFixturesDir() / "blank.osim"}; });
}

TEST(ModelWarpDocument, ConstructorThrowsIfGivenInvalidOsimPath)
{
    ASSERT_THROW({ ModelWarpDocument{std::filesystem::path{"bs.osim"}}; }, std::exception);
}

TEST(ModelWarpDocument, AfterConstructingFromBasicOsimFileTheReturnedModelContainsExpectedComponents)
{
    const ModelWarpDocument doc{GetFixturesDir() / "onebody.osim"};
    doc.model().getComponent("bodyset/some_body");
}

TEST(ModelWarpDocument, DefaultConstructedIsInAnOKState)
{
    // i.e. it is possible to warp a blank model
    const ModelWarpDocument doc;
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpDocument, BlankOsimFileIsInAnOKState)
{
    // a blank document is also warpable (albeit, trivially)
    const ModelWarpDocument doc{GetFixturesDir() / "blank.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpDocument, OneBodyIsInAnOKState)
{
    // the onebody example isn't warpable, because it can't figure out how to warp
    // the offset frame in it (the user _must_ specify that they want to ignore it, or
    // use StationDefinedFrame, etc.)
    const ModelWarpDocument doc{GetFixturesDir() / "onebody.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, SparselyNamedPairedIsInAnOKState)
{
    // the landmarks in this example are sparesely named, but fully paired, and the
    // model contains no PhysicalOffsetFrames to worry about, so it's fine
    const ModelWarpDocument doc{GetFixturesDir() / "SparselyNamedPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpDocument, SimpleUnnamedIsInAnErrorState)
{
    // the model is simple, and has landmarks on the source mesh, but there is no
    // destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    const ModelWarpDocument doc{GetFixturesDir() / "SimpleUnnamed" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, SimpleIsInAnErrorState)
{
    // the model is simple, and has named landmarks on the source mesh, but there
    // is no destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    const ModelWarpDocument doc{GetFixturesDir() / "Simple" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, PairedIsInAnOKState)
{
    // the model is simple and has fully paired meshes+landmarks: it can be warped
    const ModelWarpDocument doc{GetFixturesDir() / "Paired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpDocument, MissingSourceLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a source mesh: unwarpable
    const ModelWarpDocument doc{GetFixturesDir() / "MissingSourceLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, MissingDestinationLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a destination mesh: unwarpable
    const ModelWarpDocument doc{GetFixturesDir() / "MissingDestinationLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, PofPairedIsInAnErrorState)
{
    // the model has fully-paired meshes (good), but contains `PhysicalOffsetFrame`s
    // that haven't been explicitly handled by the user (ignored, least-squares fit, etc.)
    const ModelWarpDocument doc{GetFixturesDir() / "PofPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpDocument, WarpBlendingFactorInitiallyOne)
{
    ASSERT_EQ(ModelWarpDocument{}.getWarpBlendingFactor(), 1.0f);
}

TEST(ModelWarpDocument, WarpBlendingFactorClampedBetweenZeroAndOne)
{
    ModelWarpDocument doc;
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
    doc.setWarpBlendingFactor(5.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
    doc.setWarpBlendingFactor(-2.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 0.0f);
    doc.setWarpBlendingFactor(1.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
}

TEST(ModelWarpDocument, getShouldWriteWarpedMeshesToDisk_InitiallyFalse)
{
    // this might be important, because the UI performs _much_ better if it doesn't
    // have to write the warped meshes to disk. So it should be an explicit operation
    // when the caller (e.g. the export process) actually needs this behavior (e.g.
    // because OpenSim is going to expect on-disk mesh data)
    ASSERT_FALSE(ModelWarpDocument{}.getShouldWriteWarpedMeshesToDisk());
}

TEST(ModelWarpDocument, setShouldWriteWarpedMeshesToDisk_CanBeUsedToSetBehaviorToTrue)
{
    ModelWarpDocument doc;

    ASSERT_FALSE(doc.getShouldWriteWarpedMeshesToDisk());
    doc.setShouldWriteWarpedMeshesToDisk(true);
    ASSERT_TRUE(doc.getShouldWriteWarpedMeshesToDisk());
}

TEST(ModelWarpDocument, setShouldWriteWarpedMeshesToDisk_ChangesEquality)
{
    ModelWarpDocument a;
    ModelWarpDocument b = a;
    ASSERT_EQ(a, b);
    b.setShouldWriteWarpedMeshesToDisk(true);
    ASSERT_NE(a, b);
}

TEST(ModelWarpDocument, getWarpedMeshesOutputDirectory_ReturnsNulloptWhenNoOsimProvided)
{
    ASSERT_EQ(ModelWarpDocument{}.getWarpedMeshesOutputDirectory(), std::nullopt);
}

TEST(ModelWarpDocument, getWarpedMeshesOutputDirectory_ReturnsNonNulloptWhenOsimProvied)
{
    const std::filesystem::path fileLocation = GetFixturesDir() / "blank.osim";
    const ModelWarpDocument doc{fileLocation};
    ASSERT_NE(doc.getWarpedMeshesOutputDirectory(), std::nullopt);
}

TEST(ModelWarpDocument, getOsimFileLocation_ReturnsNulloptOnDefaultConstruction)
{
    ASSERT_EQ(ModelWarpDocument{}.getOsimFileLocation(), std::nullopt);
}

TEST(ModelWarpDocument, getOsimFileLocation_ReturnsProvidedOsimFileLocationWHenConstructedFromPath)
{
    const std::filesystem::path fileLocation = GetFixturesDir() / "blank.osim";
    const ModelWarpDocument doc{fileLocation};
    ASSERT_EQ(doc.getOsimFileLocation(), fileLocation);
}
