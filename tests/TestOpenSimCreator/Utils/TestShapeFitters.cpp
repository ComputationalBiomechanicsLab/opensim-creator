#include <OpenSimCreator/Utils/ShapeFitters.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <OpenSimCreator/Bindings/SimTKMeshLoader.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>

#include <cstdint>
#include <filesystem>

TEST(FitSphere, ReturnsUnitSphereWhenGivenAnEmptyMesh)
{
    osc::Mesh const emptyMesh;
    osc::Sphere const sphereFit = osc::FitSphere(emptyMesh);

    ASSERT_TRUE(emptyMesh.getVerts().empty());
    ASSERT_EQ(sphereFit.origin, glm::vec3(0.0f, 0.0f, 0.0f));
    ASSERT_EQ(sphereFit.radius, 1.0f);
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenAUnitSphereMesh)
{
    // generate a UV unit sphere
    osc::Mesh const sphereMesh = osc::GenSphere(16, 16);
    osc::Sphere const sphereFit = osc::FitSphere(sphereMesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.origin, glm::vec3{}, 0.000001f));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.radius, 1.0f, 0.000001f));
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenATransformedSphere)
{
    osc::Transform t;
    t.position = {7.0f, 3.0f, 1.5f};
    t.scale = {3.25f, 3.25f, 3.25f};  // keep it spherical
    t.rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3{1.0f, 1.0f, 0.0f}));

    osc::Mesh sphereMesh = osc::GenSphere(16, 16);
    sphereMesh.transformVerts(t);

    osc::Sphere const sphereFit = osc::FitSphere(sphereMesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.origin, t.position, 0.000001f));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.radius, t.scale.x, 0.000001f));
}

// reproduction: ensure the C++ rewrite produces similar results to:
//
//     How to build a dinosaur: Musculoskeletal modeling and simulation of locomotor biomechanics in extinct animals
//         Peter J. Bishop, Andrew R. Cuff, and John R. Hutchinson
//         Paleobiology, 47(1), 1-38
//         doi:10.1017/pab.2020.46
//
// that publication's supplamentary information includes the source code for
// a shape-fitting UI built in MATLAB, so you can generate reproduction test
// cases by:
//
// - downloading the supplamentary material for the paper
// - unzip it and open `doi_10.5061_dryad.73n5tb2v9__v3\MATLAB_Code\ShapeFitter\` in MATLAB
// - run `Shape_fitter.m`
// - click `Load Mesh Part`
// - load a mesh
// - fit it
// - compare the fitted analytic geometry to whatever OSC produces
TEST(FitSphere, ReturnsRoughlyTheSameAnswerForFemoralHeadAsOriginalPublishedAlgorithm)
{
    // this hard-coded result comes from running the provided `Femoral_head.obj` through the shape fitter script
    constexpr osc::Sphere c_ExpectedSphere{{5.0133f, -27.43f, 164.2998f}, 7.8291f};

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    auto const objPath =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Utils/ShapeFitting/Femoral_head.obj";
    osc::Mesh const mesh = osc::LoadMeshViaSimTK(objPath);
    osc::Sphere const sphereFit = osc::FitSphere(mesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.origin, c_ExpectedSphere.origin, 0.0001f));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.radius, c_ExpectedSphere.radius, 0.0001f));
}
