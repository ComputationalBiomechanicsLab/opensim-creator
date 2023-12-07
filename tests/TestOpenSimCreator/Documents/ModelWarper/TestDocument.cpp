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

TEST(ModelWarpingDocument, FindsAndLoadsAssociatedMeshAndSourceLandmarks)
{
    // these fixture paths were created manually
    struct Paths final {
        std::filesystem::path modelDir = GetFixturesDir() / "Simple";
        std::filesystem::path geometryDir = modelDir / "Geometry";
        std::filesystem::path osim = modelDir / "simple.osim";
        std::filesystem::path obj = geometryDir / "sphere.obj";
        std::filesystem::path landmarks = geometryDir / "sphere.landmarks.csv";
    } paths;

    Document const doc{paths.osim};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    MeshWarpPairing const* landmarks = doc.findMeshWarp(meshAbsPath);

    ASSERT_TRUE(landmarks);
    ASSERT_TRUE(landmarks->hasSourceLandmarksFile());
    ASSERT_EQ(landmarks->tryGetSourceLandmarksFile(), paths.landmarks);
    ASSERT_FALSE(landmarks->hasDestinationLandmarksFile());
    ASSERT_EQ(landmarks->getNumLandmarks(), 7);
    ASSERT_EQ(landmarks->getNumFullyPairedLandmarks(), 0);

    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(landmarks->hasLandmarkNamed(name));
    }
}
