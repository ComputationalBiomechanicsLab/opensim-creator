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

TEST(ModelWarpingDocument, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ ModelWarpDocument{}; });
}

TEST(ModelWarpingDocument, CanConstructFromPathToOsim)
{
    ASSERT_NO_THROW({ ModelWarpDocument{GetFixturesDir() / "blank.osim"}; });
}

TEST(ModelWarpingDocument, ConstructorThrowsIfGivenInvalidOsimPath)
{
    ASSERT_THROW({ ModelWarpDocument{std::filesystem::path{"bs.osim"}}; }, std::exception);
}

TEST(ModelWarpingDocument, AfterConstructingFromBasicOsimFileTheReturnedModelContainsExpectedComponents)
{
    ModelWarpDocument const doc{GetFixturesDir() / "onebody.osim"};
    doc.model().getComponent("bodyset/some_body");
}

TEST(ModelWarpingDocument, DefaultConstructedIsInAnOKState)
{
    // i.e. it is possible to warp a blank model
    ModelWarpDocument const doc;
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpingDocument, BlankOsimFileIsInAnOKState)
{
    // a blank document is also warpable (albeit, trivially)
    ModelWarpDocument const doc{GetFixturesDir() / "blank.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpingDocument, OneBodyIsInAnOKState)
{
    // the onebody example isn't warpable, because it can't figure out how to warp
    // the offset frame in it (the user _must_ specify that they want to ignore it, or
    // use StationDefinedFrame, etc.)
    ModelWarpDocument const doc{GetFixturesDir() / "onebody.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, SparselyNamedPairedIsInAnOKState)
{
    // the landmarks in this example are sparesely named, but fully paired, and the
    // model contains no PhysicalOffsetFrames to worry about, so it's fine
    ModelWarpDocument const doc{GetFixturesDir() / "SparselyNamedPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpingDocument, SimpleUnnamedIsInAnErrorState)
{
    // the model is simple, and has landmarks on the source mesh, but there is no
    // destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    ModelWarpDocument const doc{GetFixturesDir() / "SimpleUnnamed" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, SimpleIsInAnErrorState)
{
    // the model is simple, and has named landmarks on the source mesh, but there
    // is no destination mesh/landmarks, and the user hasn't specified any overrides
    // etc., so it's un-warpable
    ModelWarpDocument const doc{GetFixturesDir() / "Simple" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, PairedIsInAnOKState)
{
    // the model is simple and has fully paired meshes+landmarks: it can be warped
    ModelWarpDocument const doc{GetFixturesDir() / "Paired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Ok);
}

TEST(ModelWarpingDocument, MissingSourceLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a source mesh: unwarpable
    ModelWarpDocument const doc{GetFixturesDir() / "MissingSourceLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, MissingDestinationLMsIsInAnErrorState)
{
    // the model is simple, has source+destination meshes, but is missing landmark
    // data for a destination mesh: unwarpable
    ModelWarpDocument const doc{GetFixturesDir() / "MissingDestinationLMs" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, PofPairedIsInAnErrorState)
{
    // the model has fully-paired meshes (good), but contains `PhysicalOffsetFrame`s
    // that haven't been explicitly handled by the user (ignored, least-squares fit, etc.)
    ModelWarpDocument const doc{GetFixturesDir() / "PofPaired" / "model.osim"};
    ASSERT_EQ(doc.state(), ValidationCheckState::Error);
}

TEST(ModelWarpingDocument, WarpBlendingFactorInitiallyOne)
{
    ASSERT_EQ(ModelWarpDocument{}.getWarpBlendingFactor(), 1.0f);
}

TEST(ModelWarpingDocument, WarpBlendingFactorClampedBetweenZeroAndOne)
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
