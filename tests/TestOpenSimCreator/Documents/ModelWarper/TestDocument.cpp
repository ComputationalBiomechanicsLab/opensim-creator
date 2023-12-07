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

TEST(ModelWarpingDocument, CorrectlyLoadsSimpleCase)
{
    // the `modelDir` was created manually by first exporting a sphere + landmarks from
    // the mesh warper and composing it into a new `osim`
    struct Paths final {
        // model
        std::filesystem::path modelDir = GetFixturesDir() / "Simple";
        std::filesystem::path osim = modelDir / "model.osim";

        // source (mesh + landmarks: conventional, backwards-compatible, OpenSim Geometry dir)
        std::filesystem::path geometryDir = modelDir / "Geometry";
        std::filesystem::path obj = geometryDir / "sphere.obj";
        std::filesystem::path landmarks = geometryDir / "sphere.landmarks.csv";
    } paths;

    Document const doc{paths.osim};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    MeshWarpPairing const* pairing = doc.findMeshWarp(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were loaded...
    ASSERT_TRUE(pairing->hasSourceLandmarksFile());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_EQ(pairing->getNumLandmarks(), 7);

    // ... but no destination mesh is found...
    ASSERT_FALSE(pairing->tryGetDestinationMeshAbsoluteFilepath());

    // ... and no destination landmarks were found (not provided in this fixture)...
    ASSERT_FALSE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), 0);

    // ... and the landmarks are loaded as-expected
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        ASSERT_TRUE(pairing->tryGetLandmarkPairingByName(name));
        ASSERT_EQ(pairing->tryGetLandmarkPairingByName(name)->getName(), name);
        ASSERT_FALSE(pairing->tryGetLandmarkPairingByName(name)->isFullyPaired());  // this only tests one side of the pairing
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsPairedCase)
{
    // the `modelDir` was created manually by first exporting a sphere + landmarks from
    // the mesh warper and composing it into a new `osim`
    struct Paths final {
        // model
        std::filesystem::path modelDir = GetFixturesDir() / "Paired";
        std::filesystem::path osim = modelDir / "model.osim";

        // source (mesh + landmarks: conventional, backwards-compatible, OpenSim Geometry dir)
        std::filesystem::path geometryDir = modelDir / "Geometry";
        std::filesystem::path obj = geometryDir / "sphere.obj";
        std::filesystem::path landmarks = geometryDir / "sphere.landmarks.csv";

        // destination (mesh + landmarks: same structure as source, but reads from 'DestinationGeometry')
        std::filesystem::path destinationGeometryDir = modelDir / "DestinationGeometry";
        std::filesystem::path destinationObj = destinationGeometryDir / "sphere.obj";
        std::filesystem::path destinationLandmarks = destinationGeometryDir / "sphere.landmarks.csv";
    } paths;

    Document const doc{paths.osim};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    MeshWarpPairing const* pairing = doc.findMeshWarp(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were found ...
    ASSERT_TRUE(pairing->hasSourceLandmarksFile());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_EQ(pairing->getNumLandmarks(), 7);

    // ... and the destination mesh is correctly identified...
    ASSERT_TRUE(pairing->tryGetDestinationMeshAbsoluteFilepath());
    ASSERT_EQ(pairing->tryGetDestinationMeshAbsoluteFilepath(), paths.destinationObj);

    // ... and the destination landmarks file was found...
    ASSERT_TRUE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetDestinationLandmarksFilepath(), paths.destinationLandmarks);

    /// ... and the destination landmarks were paired with the source landmarks...
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), pairing->getNumLandmarks());

    // ... and the landmarks are loaded as-expected
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        ASSERT_TRUE(pairing->tryGetLandmarkPairingByName(name));
        ASSERT_EQ(pairing->tryGetLandmarkPairingByName(name)->getName(), name);
        ASSERT_TRUE(pairing->tryGetLandmarkPairingByName(name)->isFullyPaired());
    }
}
