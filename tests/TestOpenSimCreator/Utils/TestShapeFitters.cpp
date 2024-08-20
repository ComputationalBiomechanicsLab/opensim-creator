#include <OpenSimCreator/Utils/ShapeFitters.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>
#include <gtest/gtest.h>
#include <oscar/Graphics/Geometries/SphereGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Ellipsoid.h>
#include <oscar/Maths/EllipsoidFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec3.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <vector>

using namespace osc;
using namespace osc::literals;

TEST(FitSphere, ReturnsUnitSphereWhenGivenAnEmptyMesh)
{
    const Mesh emptyMesh;
    const Sphere sphereFit = FitSphere(emptyMesh);

    ASSERT_FALSE(emptyMesh.has_vertices());
    ASSERT_EQ(sphereFit.origin, Vec3(0.0f, 0.0f, 0.0f));
    ASSERT_EQ(sphereFit.radius, 1.0f);
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenAUnitSphereMesh)
{
    // generate a UV unit sphere
    const Mesh sphereMesh = SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}};
    const Sphere sphereFit = FitSphere(sphereMesh);

    ASSERT_TRUE(all_of(equal_within_absdiff(sphereFit.origin, Vec3{}, 0.000001f)));
    ASSERT_TRUE(equal_within_absdiff(sphereFit.radius, 1.0f, 0.000001f));
}

TEST(FitSphere, ReturnsRoughlyExpectedParametersWhenGivenATransformedSphere)
{
    Transform t;
    t.position = {7.0f, 3.0f, 1.5f};
    t.scale = {3.25f, 3.25f, 3.25f};  // keep it spherical
    t.rotation = angle_axis(45_deg, UnitVec3{1.0f, 1.0f, 0.0f});

    Mesh sphereMesh = SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}};
    sphereMesh.transform_vertices(t);

    const Sphere sphereFit = FitSphere(sphereMesh);

    ASSERT_TRUE(all_of(equal_within_absdiff(sphereFit.origin, t.position, 0.000001f)));
    ASSERT_TRUE(equal_within_reldiff(sphereFit.radius, t.scale.x, 0.000001f));
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
    constexpr Sphere c_ExpectedSphere{{5.0133f, -27.43f, 164.2998f}, 7.8291f};

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    const auto objPath =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Utils/ShapeFitting/Femoral_head.obj";
    const Mesh mesh = LoadMeshViaSimTK(objPath);
    const Sphere sphereFit = FitSphere(mesh);

    ASSERT_TRUE(all_of(equal_within_absdiff(sphereFit.origin, c_ExpectedSphere.origin, 0.0001f)));
    ASSERT_TRUE(equal_within_absdiff(sphereFit.radius, c_ExpectedSphere.radius, 0.0001f));
}

TEST(FitPlane, ReturnsUnitPlanePointingUpInYIfGivenAnEmptyMesh)
{
    const Mesh emptyMesh;
    const Plane planeFit = FitPlane(emptyMesh);

    ASSERT_FALSE(emptyMesh.has_vertices());
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
    constexpr Plane c_ExpectedPlane =
    {
        {4.6138f, -24.0131f, 163.1295f},
        {0.2131f, 0.94495f, -0.24833f},
    };

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    const auto objPath =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Utils/ShapeFitting/Femoral_head.obj";
    const Mesh mesh = LoadMeshViaSimTK(objPath);
    const Plane planeFit = FitPlane(mesh);

    ASSERT_TRUE(all_of(equal_within_absdiff(planeFit.origin, c_ExpectedPlane.origin, 0.0001f)));
    ASSERT_TRUE(all_of(equal_within_absdiff(planeFit.normal, c_ExpectedPlane.normal, 0.0001f)));
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
    constexpr Vec3 c_ExpectedOrigin = {4.41627617443540f, -28.2484366502307f, 165.041246898544f};
    constexpr Vec3 c_ExpectedRadii = {9.39508101198322f,   8.71324627349633f,  6.71387132216324f};
    // OSC change: the _signs_ of these direction vectors might be different from the MATLAB script because
    // OSC's implementation also gurantees that the vectors are right-handed
    constexpr auto c_ExpectedRadiiDirections = std::to_array<Vec3>({
        Vec3{0.387689357308333f, 0.744763303086706f, -0.543161656052074f},
        Vec3{0.343850708787853f, 0.429871105312056f, 0.834851796957929},
        Vec3{0.855256483340491f, -0.510429677030215f, -0.0894309371016929f},
    });
    constexpr float c_MaximumAbsoluteError = 0.0001f;

    // Femoral_head.obj is copied from the example data that came with the supplamentary information
    const auto objPath =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Utils/ShapeFitting/Femoral_head.obj";
    const Mesh mesh = LoadMeshViaSimTK(objPath);
    const Ellipsoid fit = FitEllipsoid(mesh);
    const auto directions = axis_directions_of(fit);

    ASSERT_TRUE(all_of(equal_within_absdiff(fit.origin, c_ExpectedOrigin, c_MaximumAbsoluteError)));
    ASSERT_TRUE(all_of(equal_within_absdiff(fit.radii,  c_ExpectedRadii, c_MaximumAbsoluteError)));
    ASSERT_TRUE(all_of(equal_within_absdiff(directions[0], c_ExpectedRadiiDirections[0], c_MaximumAbsoluteError)));
    ASSERT_TRUE(all_of(equal_within_absdiff(directions[1], c_ExpectedRadiiDirections[1], c_MaximumAbsoluteError)));
    ASSERT_TRUE(all_of(equal_within_absdiff(directions[2], c_ExpectedRadiiDirections[2], c_MaximumAbsoluteError)));
}

TEST(FitEllipsoid, ThrowsErrorIfGivenLessThan9Points)
{
    const auto generateSphericalMeshWithNPoints = [](size_t n)
    {
        Radians theta{0.0f};
        Radians phi{0.0f};
        const float radius = 1.0f;

        std::vector<Vec3> vertices(n);
        for (Vec3& vertex : vertices) {
            vertex.x = radius * sin(theta) * cos(phi);
            vertex.y = radius * sin(theta);
            vertex.z = radius * cos(theta) * cos(phi);
            theta += 360_deg / static_cast<float>(n);
            phi += 360_deg / static_cast<float>(n);
        }
        std::vector<uint16_t> indices(n);
        std::iota(indices.begin(), indices.end(), static_cast<uint16_t>(0));

        Mesh m;
        m.set_vertices(vertices);
        m.set_indices(indices);
        return m;
    };

    for (size_t i = 0; i < 9; ++i)
    {
        ASSERT_ANY_THROW({ FitEllipsoid(generateSphericalMeshWithNPoints(i)); });
    }

    // shouldn't throw
    FitEllipsoid(generateSphericalMeshWithNPoints(9));
    FitEllipsoid(generateSphericalMeshWithNPoints(10));
}
