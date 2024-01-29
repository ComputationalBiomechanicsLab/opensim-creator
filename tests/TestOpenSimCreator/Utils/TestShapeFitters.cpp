#include <OpenSimCreator/Utils/ShapeFitters.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <gtest/gtest.h>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Ellipsoid.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <vector>

using namespace osc::literals;

using osc::Vec3;

TEST(FitSphere, ReturnsUnitSphereWhenGivenAnEmptyMesh)
{
    osc::Mesh const emptyMesh;
    osc::Sphere const sphereFit = osc::FitSphere(emptyMesh);

    ASSERT_FALSE(emptyMesh.hasVerts());
    ASSERT_EQ(sphereFit.origin, Vec3(0.0f, 0.0f, 0.0f));
    ASSERT_EQ(sphereFit.radius, 1.0f);
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenAUnitSphereMesh)
{
    // generate a UV unit sphere
    osc::Mesh const sphereMesh = osc::GenerateUVSphereMesh(16, 16);
    osc::Sphere const sphereFit = osc::FitSphere(sphereMesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.origin, Vec3{}, 0.000001f));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(sphereFit.radius, 1.0f, 0.000001f));
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenATransformedSphere)
{
    osc::Transform t;
    t.position = {7.0f, 3.0f, 1.5f};
    t.scale = {3.25f, 3.25f, 3.25f};  // keep it spherical
    t.rotation = osc::AngleAxis(45_deg, osc::Normalize(Vec3{1.0f, 1.0f, 0.0f}));

    osc::Mesh sphereMesh = osc::GenerateUVSphereMesh(16, 16);
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

TEST(FitPlane, ReturnsUnitPlanePointingUpInYIfGivenAnEmptyMesh)
{
    osc::Mesh const emptyMesh;
    osc::Plane const planeFit = osc::FitPlane(emptyMesh);

    ASSERT_FALSE(emptyMesh.hasVerts());
    ASSERT_EQ(planeFit.origin, Vec3(0.0f, 0.0f, 0.0f));
    ASSERT_EQ(planeFit.normal, Vec3(0.0f, 1.0f, 0.0f));
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
TEST(FitPlane, ReturnsRoughlyTheSameAnswerForFemoralHeadAsOriginalPublishedAlgorithm)
{
    // this hard-coded result comes from running the provided `Femoral_head.obj` through the shape fitter script
    constexpr osc::Plane c_ExpectedPlane =
    {
        {4.6138f, -24.0131f, 163.1295f},
        {0.2131f, 0.94495f, -0.24833f},
    };

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    auto const objPath =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Utils/ShapeFitting/Femoral_head.obj";
    osc::Mesh const mesh = osc::LoadMeshViaSimTK(objPath);
    osc::Plane const planeFit = osc::FitPlane(mesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(planeFit.origin, c_ExpectedPlane.origin, 0.0001f));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(planeFit.normal, c_ExpectedPlane.normal, 0.0001f));
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
TEST(FitEllipsoid, ReturnsRoughlyTheSameAnswerForFemoralHeadAsOriginalPublishedAlgorithm)
{
    // this hard-coded result comes from running the provided `Femoral_head.obj` through the shape fitter script
    constexpr osc::Ellipsoid c_ExpectedFit =
    {
        {4.41627617443540f, -28.2484366502307f, 165.041246898544f},
        {9.39508101198322f,   8.71324627349633f,  6.71387132216324f},

        // OSC change: the _signs_ of these direction vectors might be different from the MATLAB script because
        // OSC's implementation also gurantees that the vectors are right-handed
        std::to_array<Vec3>
        ({
            Vec3{0.387689357308333f, 0.744763303086706f, -0.543161656052074f},
            Vec3{0.343850708787853f, 0.429871105312056f, 0.834851796957929},
            Vec3{0.855256483340491f, -0.510429677030215f, -0.0894309371016929f},
        })
    };
    constexpr float c_MaximumAbsoluteError = 0.0001f;

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    auto const objPath =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Utils/ShapeFitting/Femoral_head.obj";
    osc::Mesh const mesh = osc::LoadMeshViaSimTK(objPath);
    osc::Ellipsoid const fit = osc::FitEllipsoid(mesh);

    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(fit.origin, c_ExpectedFit.origin, c_MaximumAbsoluteError));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(fit.radii, c_ExpectedFit.radii, c_MaximumAbsoluteError));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(fit.radiiDirections[0], c_ExpectedFit.radiiDirections[0], c_MaximumAbsoluteError));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(fit.radiiDirections[1], c_ExpectedFit.radiiDirections[1], c_MaximumAbsoluteError));
    ASSERT_TRUE(osc::IsEqualWithinAbsoluteError(fit.radiiDirections[2], c_ExpectedFit.radiiDirections[2], c_MaximumAbsoluteError));
}

TEST(FitEllipsoid, DISABLED_ThrowsErrorIfGivenLessThan9Points)
{
    auto const generateMeshWithNPoints = [](size_t n)
    {
        std::vector<Vec3> verts(n);
        std::vector<uint16_t> indices(n);
        std::iota(indices.begin(), indices.end(), static_cast<uint16_t>(0));

        osc::Mesh m;
        m.setVerts(verts);
        m.setIndices(indices);
        return m;
    };

    for (size_t i = 0; i < 9; ++i)
    {
        ASSERT_ANY_THROW({ generateMeshWithNPoints(i); });
    }

    // shouldn't throw
    generateMeshWithNPoints(9);
    generateMeshWithNPoints(10);
}
