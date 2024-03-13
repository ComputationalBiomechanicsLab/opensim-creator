#include <OpenSimCreator/Documents/ModelWarper/PointWarperFactories.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/TPSLandmarkPairWarperFactory.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

using namespace osc::mow;

namespace
{
    std::filesystem::path ModelWarperFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(PointWarperFactories, CorrectlyLoadsSimpleCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "Simple";
        std::filesystem::path osim = modelDir / "model.osim";

        // source (mesh + landmarks: conventional, backwards-compatible, OpenSim Geometry dir)
        std::filesystem::path geometryDir = modelDir / "Geometry";
        std::filesystem::path obj = geometryDir / "sphere.obj";
        std::filesystem::path landmarks = geometryDir / "sphere.landmarks.csv";
    } paths;

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were loaded...
    ASSERT_TRUE(pairing->hasSourceLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_TRUE(pairing->hasSourceLandmarks());
    ASSERT_EQ(pairing->getNumSourceLandmarks(), 7);

    // ... but no destination mesh is found...
    ASSERT_FALSE(pairing->tryGetDestinationMeshAbsoluteFilepath());

    // ... and no destination landmarks were found (not provided in this fixture)...
    ASSERT_FALSE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_FALSE(pairing->hasDestinationLandmarks());
    ASSERT_EQ(pairing->getNumDestinationLandmarks(), 0);

    // ... therefore, every landmark is unpaired...
    ASSERT_EQ(pairing->getNumLandmarks(), pairing->getNumUnpairedLandmarks());
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), 0);

    // ... and the (partial) landmarks are loaded as-expected
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        auto p = pairing->tryGetLandmarkPairingByName(name);

        ASSERT_TRUE(p);
        ASSERT_EQ(p->name(), name);
        ASSERT_TRUE(p->hasSource());
        ASSERT_FALSE(p->hasDestination());
        ASSERT_FALSE(p->isFullyPaired());  // this only tests one side of the pairing
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsPairedCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "Paired";
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

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh filepath is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were found ...
    ASSERT_TRUE(pairing->hasSourceLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_TRUE(pairing->hasSourceLandmarks());
    ASSERT_EQ(pairing->getNumSourceLandmarks(), 7);

    // ... and the destination mesh filepath is correctly identified...
    ASSERT_TRUE(pairing->hasDestinationMeshFilepath());
    ASSERT_EQ(pairing->tryGetDestinationMeshAbsoluteFilepath(), paths.destinationObj);

    // ... and the destination landmarks file was found & loaded ...
    ASSERT_TRUE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetDestinationLandmarksFilepath(), paths.destinationLandmarks);
    ASSERT_TRUE(pairing->hasDestinationLandmarks());
    ASSERT_EQ(pairing->getNumDestinationLandmarks(), 7);

    /// ... and all landmarks are fully paired...
    ASSERT_EQ(pairing->getNumLandmarks(), pairing->getNumFullyPairedLandmarks());
    ASSERT_FALSE(pairing->hasUnpairedLandmarks());
    ASSERT_EQ(pairing->getNumUnpairedLandmarks(), 0);

    // ... and the loaded landmark pairs are as-expected
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        auto p = pairing->tryGetLandmarkPairingByName(name);

        ASSERT_TRUE(p);
        ASSERT_EQ(p->name(), name);
        ASSERT_TRUE(p->hasSource());
        ASSERT_TRUE(p->hasDestination());
        ASSERT_TRUE(p->isFullyPaired());
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsMissingDestinationLMsCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "MissingDestinationLMs";
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

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were found ...
    ASSERT_TRUE(pairing->hasSourceLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_TRUE(pairing->hasSourceLandmarks());
    ASSERT_EQ(pairing->getNumLandmarks(), 7);

    // ... and the destination mesh file is correctly identified...
    ASSERT_TRUE(pairing->tryGetDestinationMeshAbsoluteFilepath());
    ASSERT_EQ(pairing->tryGetDestinationMeshAbsoluteFilepath(), paths.destinationObj);

    // ... but the destination landmarks are not found...
    ASSERT_FALSE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_FALSE(pairing->tryGetDestinationLandmarksFilepath().has_value());

    // ... (you can still ask where the destination landmarks file _should_ be, though)...
    ASSERT_EQ(pairing->recommendedDestinationLandmarksFilepath(), paths.destinationLandmarks);

    // ... therefore, all landmarks are unpaired
    ASSERT_TRUE(pairing->hasUnpairedLandmarks());
    ASSERT_EQ(pairing->getNumLandmarks(), pairing->getNumUnpairedLandmarks());
    ASSERT_FALSE(pairing->hasDestinationLandmarks());
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), 0);

    // ... and the landmarks are loaded one-sided
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        auto p = pairing->tryGetLandmarkPairingByName(name);

        ASSERT_TRUE(p);
        ASSERT_EQ(p->name(), name);
        ASSERT_TRUE(p->hasSource());
        ASSERT_FALSE(p->hasDestination());
        ASSERT_FALSE(p->isFullyPaired());
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsMissingSourceLMsCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "MissingSourceLMs";
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

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh file is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... but no source landmarks file was found ...
    ASSERT_FALSE(pairing->hasSourceLandmarksFilepath());
    ASSERT_FALSE(pairing->tryGetSourceLandmarksFilepath().has_value());
    ASSERT_FALSE(pairing->hasSourceLandmarks());

    // ... (you can still ask where the destination landmarks file _should_ be, though)...
    ASSERT_EQ(pairing->recommendedSourceLandmarksFilepath(), paths.landmarks);

    // ... the destination mesh file is correctly identified...
    ASSERT_TRUE(pairing->tryGetDestinationMeshAbsoluteFilepath());
    ASSERT_EQ(pairing->tryGetDestinationMeshAbsoluteFilepath(), paths.destinationObj);

    // ... the destination landmarks file is found...
    ASSERT_TRUE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetDestinationLandmarksFilepath(), paths.destinationLandmarks);

    // ... so destination landmarks are available ...
    ASSERT_TRUE(pairing->hasDestinationLandmarks());
    ASSERT_EQ(pairing->getNumDestinationLandmarks(), 7);
    ASSERT_EQ(pairing->getNumLandmarks(), pairing->getNumDestinationLandmarks());;

    // ... but all landmarks are unpaired...
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), 0);
    ASSERT_EQ(pairing->getNumUnpairedLandmarks(), pairing->getNumLandmarks());
    ASSERT_TRUE(pairing->hasUnpairedLandmarks());

    // ... and the landmarks are loaded one-sided (destination only)
    for (auto const& name : {"landmark_0", "landmark_2", "landmark_5", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name));
        auto p = pairing->tryGetLandmarkPairingByName(name);
        ASSERT_TRUE(p);
        ASSERT_EQ(p->name(), name);
        ASSERT_FALSE(p->hasSource());
        ASSERT_TRUE(p->hasDestination());
        ASSERT_FALSE(p->isFullyPaired());
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsSimpleUnnamedCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "SimpleUnnamed";
        std::filesystem::path osim = modelDir / "model.osim";

        // source (mesh + landmarks: conventional, backwards-compatible, OpenSim Geometry dir)
        std::filesystem::path geometryDir = modelDir / "Geometry";
        std::filesystem::path obj = geometryDir / "sphere.obj";
        std::filesystem::path landmarks = geometryDir / "sphere.landmarks.csv";
    } paths;

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were found ...
    ASSERT_TRUE(pairing->hasSourceLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_TRUE(pairing->hasSourceLandmarks());
    ASSERT_EQ(pairing->getNumLandmarks(), 7);

    // ... but no destination mesh/landmarks were found...
    ASSERT_FALSE(pairing->hasDestinationMeshFilepath());
    ASSERT_FALSE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_FALSE(pairing->hasDestinationLandmarks());

    // ... so the landmarks are unpaired...
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), 0);
    ASSERT_EQ(pairing->getNumUnpairedLandmarks(), pairing->getNumLandmarks());

    // ... and, because the landmarks were unnamed, they were assigned  a name of `unnamed_$i`
    for (auto const& name : {"unnamed_0", "unnamed_1", "unnamed_2", "unnamed_3"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name)) << name;
        auto p = pairing->tryGetLandmarkPairingByName(name);
        ASSERT_TRUE(p) << name;
        ASSERT_EQ(p->name(), name);
        ASSERT_TRUE(p->hasSource());
        ASSERT_FALSE(p->hasDestination());
        ASSERT_FALSE(p->isFullyPaired());
    }
}

TEST(ModelWarpingDocument, CorrectlyLoadsSparselyNamedPairedCase)
{
    struct Paths final {
        // model
        std::filesystem::path modelDir = ModelWarperFixturesDir() / "SparselyNamedPaired";
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

    OpenSim::Model const model{paths.osim.string()};
    PointWarperFactories const lut{paths.osim, model, ModelWarpConfiguration{paths.osim, model}};
    std::string const meshAbsPath = "/bodyset/new_body/new_body_geom_1";
    auto const* pairing = lut.find<TPSLandmarkPairWarperFactory>(meshAbsPath);

    // the pairing is found...
    ASSERT_TRUE(pairing);

    // ... and the source mesh file is correctly identified...
    ASSERT_EQ(pairing->getSourceMeshAbsoluteFilepath(), paths.obj);

    // ... and source landmarks were found ...
    ASSERT_TRUE(pairing->hasSourceLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetSourceLandmarksFilepath(), paths.landmarks);
    ASSERT_TRUE(pairing->hasSourceLandmarks());
    ASSERT_EQ(pairing->getNumSourceLandmarks(), 7);
    ASSERT_EQ(pairing->getNumLandmarks(), 7);

    // ... and the destination mesh is correctly identified...
    ASSERT_TRUE(pairing->tryGetDestinationMeshAbsoluteFilepath());
    ASSERT_EQ(pairing->tryGetDestinationMeshAbsoluteFilepath(), paths.destinationObj);

    // ... and the destination landmarks file was found...
    ASSERT_TRUE(pairing->hasDestinationLandmarksFilepath());
    ASSERT_EQ(pairing->tryGetDestinationLandmarksFilepath(), paths.destinationLandmarks);

    /// ... and the destination landmarks were loaded correctly paired with the source landmarks...
    ASSERT_TRUE(pairing->hasDestinationLandmarks());
    ASSERT_EQ(pairing->getNumDestinationLandmarks(), 7);
    ASSERT_EQ(pairing->getNumFullyPairedLandmarks(), pairing->getNumLandmarks());

    // ... named elements were able to be paired out-of-order, unnamed elements were paired in-order...
    for (auto const& name : {"landmark_0", "unnamed_0", "unnamed_1", "landmark_3", "landmark_4", "unnamed_2", "landmark_6"})
    {
        ASSERT_TRUE(pairing->hasLandmarkNamed(name)) << name;
        auto p = pairing->tryGetLandmarkPairingByName(name);

        ASSERT_TRUE(p);
        ASSERT_EQ(p->name(), name);
        ASSERT_TRUE(p->isFullyPaired());
    }
}
