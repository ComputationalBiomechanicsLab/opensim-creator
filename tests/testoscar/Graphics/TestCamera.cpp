#include <oscar/Graphics/Camera.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/CameraProjection.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>

#include <array>
#include <cstddef>
#include <sstream>
#include <utility>

using namespace osc;
using namespace osc::literals;
using namespace osc::testing;

TEST(Camera, CanDefaultConstruct)
{
    Camera camera;  // should compile + run
}

TEST(Camera, CanCopyConstruct)
{
    Camera camera;
    Camera{camera};
}

TEST(Camera, CopyConstructedComparesEqual)
{
    Camera camera;
    Camera copy{camera};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(camera, copy);
}

TEST(Camera, CanMoveConstruct)
{
    Camera camera;
    Camera copy{std::move(camera)};
}

TEST(Camera, CanCopyAssign)
{
    Camera c1;
    Camera c2;

    c2 = c1;
}

TEST(Camera, CopyAssignedComparesEqualToSource)
{
    Camera c1;
    Camera c2;

    c1 = c2;

    ASSERT_EQ(c1, c2);
}

TEST(Camera, CanMoveAssign)
{
    Camera c1;
    Camera c2;

    c2 = std::move(c1);
}

TEST(Camera, UsesValueComparison)
{
    Camera c1;
    Camera c2;

    ASSERT_EQ(c1, c2);

    c1.set_vertical_fov(1337_deg);

    ASSERT_NE(c1, c2);

    c2.set_vertical_fov(1337_deg);

    ASSERT_EQ(c1, c2);
}

TEST(Camera, ResetResetsToDefaultValues)
{
    const Camera default_camera;
    Camera camera = default_camera;
    camera.set_direction({1.0f, 0.0f, 0.0f});
    ASSERT_NE(camera, default_camera);
    camera.reset();
    ASSERT_EQ(camera, default_camera);
}

TEST(Camera, CanGetBackgroundColor)
{
    Camera camera;

    ASSERT_EQ(camera.background_color(), Color::clear());
}

TEST(Camera, CanSetBackgroundColor)
{
    Camera camera;
    camera.set_background_color(GenerateColor());
}

TEST(Camera, SetBackgroundColorMakesGetBackgroundColorReturnTheColor)
{
    Camera camera;
    const Color color = GenerateColor();

    camera.set_background_color(color);

    ASSERT_EQ(camera.background_color(), color);
}

TEST(Camera, SetBackgroundColorMakesCameraCompareNonEqualWithCopySource)
{
    Camera camera;
    Camera copy{camera};

    ASSERT_EQ(camera, copy);

    copy.set_background_color(GenerateColor());

    ASSERT_NE(camera, copy);
}

TEST(Camera, GetClearFlagsReturnsColorAndDepthOnDefaultConstruction)
{
    Camera camera;

    ASSERT_TRUE(camera.clear_flags() & CameraClearFlags::SolidColor);
    ASSERT_TRUE(camera.clear_flags() & CameraClearFlags::Depth);
}

TEST(Camera, SetClearFlagsWorksAsExpected)
{
    Camera camera;

    const auto flagsToTest = std::to_array({
        CameraClearFlags::SolidColor,
        CameraClearFlags::Depth,
        CameraClearFlags::SolidColor | CameraClearFlags::Depth,
    });

    for (CameraClearFlags flags : flagsToTest) {
        camera.set_clear_flags(flags);
        ASSERT_EQ(camera.clear_flags(), flags);
    }
}

TEST(Camera, GetCameraProjectionReturnsProject)
{
    Camera camera;
    ASSERT_EQ(camera.projection(), CameraProjection::Perspective);
}

TEST(Camera, CanSetCameraProjection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
}

TEST(Camera, SetCameraProjectionMakesGetCameraProjectionReturnSetProjection)
{
    Camera camera;
    CameraProjection proj = CameraProjection::Orthographic;

    ASSERT_NE(camera.projection(), proj);

    camera.set_projection(proj);

    ASSERT_EQ(camera.projection(), proj);
}

TEST(Camera, SetCameraProjectionMakesCameraCompareNotEqual)
{
    Camera camera;
    Camera copy{camera};
    CameraProjection proj = CameraProjection::Orthographic;

    ASSERT_NE(copy.projection(), proj);

    copy.set_projection(proj);

    ASSERT_NE(camera, copy);
}

TEST(Camera, GetPositionReturnsOriginOnDefaultConstruction)
{
    Camera camera;
    ASSERT_EQ(camera.position(), Vec3(0.0f, 0.0f, 0.0f));
}

TEST(Camera, SetDirectionToStandardDirectionCausesGetDirectionToReturnTheDirection)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    Camera camera;

    Vec3 defaultDirection = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.direction(), defaultDirection);

    Vec3 differentDirection = normalize(Vec3{1.0f, 2.0f, -0.5f});
    camera.set_direction(differentDirection);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.direction() == differentDirection);  // just ensure it compiles

    camera.set_direction(defaultDirection);

    ASSERT_EQ(camera.direction(), defaultDirection);
}

TEST(Camera, SetDirectionToDifferentDirectionGivesAccurateEnoughResults)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    Camera camera;

    Vec3 newDirection = normalize(Vec3{1.0f, 1.0f, 1.0f});

    camera.set_direction(newDirection);

    Vec3 returnedDirection = camera.direction();

    ASSERT_GT(dot(newDirection, returnedDirection), 0.999f);
}

TEST(Camera, GetViewMatrixReturnsViewMatrixBasedOnPositonDirectionAndUp)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({0.0f, 0.0f, 0.0f});

    Mat4 viewMatrix = camera.view_matrix();
    Mat4 expectedMatrix = identity<Mat4>();

    ASSERT_EQ(viewMatrix, expectedMatrix);
}

TEST(Camera, SetViewMatrixOverrideSetsANewViewMatrixThatCanBeRetrievedWithGetViewMatrix)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Mat4 viewMatrix = identity<Mat4>();
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(viewMatrix);

    ASSERT_EQ(camera.view_matrix(), viewMatrix);
}

TEST(Camera, SetViewMatrixOverrideNulloptResetsTheViewMatrixToUsingStandardCameraPositionEtc)
{
    Camera camera;
    Mat4 initialViewMatrix = camera.view_matrix();

    Mat4 viewMatrix = identity<Mat4>();
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(viewMatrix);
    ASSERT_NE(camera.view_matrix(), initialViewMatrix);
    ASSERT_EQ(camera.view_matrix(), viewMatrix);

    camera.set_view_matrix_override(std::nullopt);

    ASSERT_EQ(camera.view_matrix(), initialViewMatrix);
}

TEST(Camera, GetProjectionMatrixReturnsProjectionMatrixBasedOnPositonDirectionAndUp)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({0.0f, 0.0f, 0.0f});

    Mat4 mtx = camera.projection_matrix(1.0f);
    Mat4 expected = identity<Mat4>();

    // only compare the Y, Z, and W columns: the X column depends on the aspect ratio of the output
    // target
    ASSERT_EQ(mtx[1], expected[1]);
    ASSERT_EQ(mtx[2], expected[2]);
    ASSERT_EQ(mtx[3], expected[3]);
}

TEST(Camera, SetProjectionMatrixOverrideSetsANewProjectionMatrixThatCanBeRetrievedWithGetProjectionMatrix)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Mat4 ProjectionMatrix = identity<Mat4>();
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(ProjectionMatrix);

    ASSERT_EQ(camera.projection_matrix(1.0f), ProjectionMatrix);
}

TEST(Camera, SetProjectionMatrixNulloptResetsTheProjectionMatrixToUsingStandardCameraPositionEtc)
{
    Camera camera;
    Mat4 initialProjectionMatrix = camera.projection_matrix(1.0f);

    Mat4 ProjectionMatrix = identity<Mat4>();
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(ProjectionMatrix);
    ASSERT_NE(camera.projection_matrix(1.0f), initialProjectionMatrix);
    ASSERT_EQ(camera.projection_matrix(1.0f), ProjectionMatrix);

    camera.set_projection_matrix_override(std::nullopt);

    ASSERT_EQ(camera.projection_matrix(1.0f), initialProjectionMatrix);
}

TEST(Camera, GetViewProjectionMatrixReturnsViewMatrixMultipliedByProjectionMatrix)
{
    Camera camera;

    Mat4 viewMatrix = identity<Mat4>();
    viewMatrix[0][3] = 2.5f;  // change some part of it

    Mat4 projectionMatrix = identity<Mat4>();
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    Mat4 expected = projectionMatrix * viewMatrix;

    camera.set_view_matrix_override(viewMatrix);
    camera.set_projection_matrix_override(projectionMatrix);

    ASSERT_EQ(camera.view_projection_matrix(1.0f), expected);
}

TEST(Camera, GetInverseViewProjectionMatrixReturnsExpectedAnswerWhenUsingOverriddenMatrices)
{
    Camera camera;

    Mat4 viewMatrix = identity<Mat4>();
    viewMatrix[0][3] = 2.5f;  // change some part of it

    Mat4 projectionMatrix = identity<Mat4>();
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    Mat4 expected = inverse(projectionMatrix * viewMatrix);

    camera.set_view_matrix_override(viewMatrix);
    camera.set_projection_matrix_override(projectionMatrix);

    ASSERT_EQ(camera.inverse_view_projection_matrix(1.0f), expected);
}

TEST(Camera, GetClearFlagsReturnsDefaultOnDefaultConstruction)
{
    Camera camera;
    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);
}

TEST(Camera, SetClearFlagsCausesGetClearFlagsToReturnNewValue)
{
    Camera camera;

    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);

    camera.set_clear_flags(CameraClearFlags::Nothing);

    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Nothing);
}

TEST(Camera, SetClearFlagsCausesCopyToReturnNonEqual)
{
    Camera camera;
    Camera copy{camera};

    ASSERT_EQ(camera, copy);
    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);

    camera.set_clear_flags(CameraClearFlags::Nothing);

    ASSERT_NE(camera, copy);
}
