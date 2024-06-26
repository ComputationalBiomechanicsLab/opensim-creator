#include <OpenSimCreator/Documents/ModelWarper/WarpableModel.h>

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

TEST(WarpableModel, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ WarpableModel{}; });
}

TEST(WarpableModel, CanConstructFromPathToOsim)
{
    ASSERT_NO_THROW({ WarpableModel{GetFixturesDir() / "blank.osim"}; });
}

TEST(WarpableModel, ConstructorThrowsIfGivenInvalidOsimPath)
{
    ASSERT_THROW({ WarpableModel{std::filesystem::path{"bs.osim"}}; }, std::exception);
}

TEST(WarpableModel, AfterConstructingFromBasicOsimFileTheReturnedModelContainsExpectedComponents)
{
    const WarpableModel doc{GetFixturesDir() / "onebody.osim"};
    doc.model().getComponent("bodyset/some_body");
}

TEST(WarpableModel, DefaultConstructedIsInAnOKState)
{
    // i.e. it is possible to warp a blank model
    const WarpableModel doc;
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(WarpableModel, BlankOsimFileIsInAnOKState)
{
    // a blank document is also warpable (albeit, trivially)
    const WarpableModel doc{GetFixturesDir() / "blank.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(WarpableModel, OneBodyIsInAnOKState)
{
    // the onebody example isn't warpable, because it can't figure out how to warp
    // the offset frame in it (the user _must_ specify that they want to ignore it, or
    // use StationDefinedFrame, etc.)
    const WarpableModel doc{GetFixturesDir() / "onebody.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, SparselyNamedPairedIsInAnOKState)
{
    // the landmarks in this example are sparesely named, but fully paired, and the
    // model contains no PhysicalOffsetFrames to worry about, so it's fine
    const WarpableModel doc{GetFixturesDir() / "SparselyNamedPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(WarpableModel, SimpleUnnamedIsInAnErrorState)
{
    // the model is simple, and has landmarks on the source mesh, but there is no
    // destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    const WarpableModel doc{GetFixturesDir() / "SimpleUnnamed" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, SimpleIsInAnErrorState)
{
    // the model is simple, and has named landmarks on the source mesh, but there
    // is no destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    const WarpableModel doc{GetFixturesDir() / "Simple" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, PairedIsInAnOKState)
{
    // the model is simple and has fully paired meshes+landmarks: it can be warped
    const WarpableModel doc{GetFixturesDir() / "Paired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(WarpableModel, MissingSourceLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a source mesh: unwarpable
    const WarpableModel doc{GetFixturesDir() / "MissingSourceLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, MissingDestinationLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a destination mesh: unwarpable
    const WarpableModel doc{GetFixturesDir() / "MissingDestinationLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, PofPairedIsInAnErrorState)
{
    // the model has fully-paired meshes (good), but contains `PhysicalOffsetFrame`s
    // that haven't been explicitly handled by the user (ignored, least-squares fit, etc.)
    const WarpableModel doc{GetFixturesDir() / "PofPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(WarpableModel, WarpBlendingFactorInitiallyOne)
{
    ASSERT_EQ(WarpableModel{}.getWarpBlendingFactor(), 1.0f);
}

TEST(WarpableModel, WarpBlendingFactorClampedBetweenZeroAndOne)
{
    WarpableModel doc;
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
    doc.setWarpBlendingFactor(5.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
    doc.setWarpBlendingFactor(-2.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 0.0f);
    doc.setWarpBlendingFactor(1.0f);
    ASSERT_EQ(doc.getWarpBlendingFactor(), 1.0f);
}

TEST(WarpableModel, getShouldWriteWarpedMeshesToDisk_InitiallyFalse)
{
    // this might be important, because the UI performs _much_ better if it doesn't
    // have to write the warped meshes to disk. So it should be an explicit operation
    // when the caller (e.g. the export process) actually needs this behavior (e.g.
    // because OpenSim is going to expect on-disk mesh data)
    ASSERT_FALSE(WarpableModel{}.getShouldWriteWarpedMeshesToDisk());
}

TEST(WarpableModel, setShouldWriteWarpedMeshesToDisk_CanBeUsedToSetBehaviorToTrue)
{
    WarpableModel doc;

    ASSERT_FALSE(doc.getShouldWriteWarpedMeshesToDisk());
    doc.setShouldWriteWarpedMeshesToDisk(true);
    ASSERT_TRUE(doc.getShouldWriteWarpedMeshesToDisk());
}

TEST(WarpableModel, setShouldWriteWarpedMeshesToDisk_ChangesEquality)
{
    WarpableModel a;
    WarpableModel b = a;
    ASSERT_EQ(a, b);
    b.setShouldWriteWarpedMeshesToDisk(true);
    ASSERT_NE(a, b);
}

TEST(WarpableModel, getWarpedMeshesOutputDirectory_ReturnsNulloptWhenNoOsimProvided)
{
    ASSERT_EQ(WarpableModel{}.getWarpedMeshesOutputDirectory(), std::nullopt);
}

TEST(WarpableModel, getWarpedMeshesOutputDirectory_ReturnsNonNulloptWhenOsimProvied)
{
    const std::filesystem::path fileLocation = GetFixturesDir() / "blank.osim";
    const WarpableModel doc{fileLocation};
    ASSERT_NE(doc.getWarpedMeshesOutputDirectory(), std::nullopt);
}

TEST(WarpableModel, getOsimFileLocation_ReturnsNulloptOnDefaultConstruction)
{
    ASSERT_EQ(WarpableModel{}.getOsimFileLocation(), std::nullopt);
}

TEST(WarpableModel, getOsimFileLocation_ReturnsProvidedOsimFileLocationWHenConstructedFromPath)
{
    const std::filesystem::path fileLocation = GetFixturesDir() / "blank.osim";
    const WarpableModel doc{fileLocation};
    ASSERT_EQ(doc.getOsimFileLocation(), fileLocation);
}
