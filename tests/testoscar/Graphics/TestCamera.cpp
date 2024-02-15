#include <oscar/Graphics/Camera.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/CameraProjection.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Angle.h>
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
    Camera c;
    Camera{c};
}

TEST(Camera, CopyConstructedComparesEqual)
{
    Camera c;
    Camera copy{c};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(c, copy);
}

TEST(Camera, CanMoveConstruct)
{
    Camera c;
    Camera copy{std::move(c)};
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

    c1.setVerticalFOV(1337_deg);

    ASSERT_NE(c1, c2);

    c2.setVerticalFOV(1337_deg);

    ASSERT_EQ(c1, c2);
}

TEST(Camera, ResetResetsToDefaultValues)
{
    Camera const defaultCamera;
    Camera camera = defaultCamera;
    camera.setDirection({1.0f, 0.0f, 0.0f});
    ASSERT_NE(camera, defaultCamera);
    camera.reset();
    ASSERT_EQ(camera, defaultCamera);
}

TEST(Camera, CanGetBackgroundColor)
{
    Camera camera;

    ASSERT_EQ(camera.getBackgroundColor(), Color::clear());
}

TEST(Camera, CanSetBackgroundColor)
{
    Camera camera;
    camera.setBackgroundColor(GenerateColor());
}

TEST(Camera, SetBackgroundColorMakesGetBackgroundColorReturnTheColor)
{
    Camera camera;
    Color const color = GenerateColor();

    camera.setBackgroundColor(color);

    ASSERT_EQ(camera.getBackgroundColor(), color);
}

TEST(Camera, SetBackgroundColorMakesCameraCompareNonEqualWithCopySource)
{
    Camera camera;
    Camera copy{camera};

    ASSERT_EQ(camera, copy);

    copy.setBackgroundColor(GenerateColor());

    ASSERT_NE(camera, copy);
}

TEST(Camera, GetClearFlagsReturnsColorAndDepthOnDefaultConstruction)
{
    Camera camera;

    ASSERT_TRUE(camera.getClearFlags() & CameraClearFlags::SolidColor);
    ASSERT_TRUE(camera.getClearFlags() & CameraClearFlags::Depth);
}

TEST(Camera, SetClearFlagsWorksAsExpected)
{
    Camera camera;

    auto const flagsToTest = std::to_array(
    {
        CameraClearFlags::SolidColor,
        CameraClearFlags::Depth,
        CameraClearFlags::SolidColor | CameraClearFlags::Depth,
    });

    for (CameraClearFlags flags : flagsToTest)
    {
        camera.setClearFlags(flags);
        ASSERT_EQ(camera.getClearFlags(), flags);
    }
}

TEST(Camera, GetCameraProjectionReturnsProject)
{
    Camera camera;
    ASSERT_EQ(camera.getCameraProjection(), CameraProjection::Perspective);
}

TEST(Camera, CanSetCameraProjection)
{
    Camera camera;
    camera.setCameraProjection(CameraProjection::Orthographic);
}

TEST(Camera, SetCameraProjectionMakesGetCameraProjectionReturnSetProjection)
{
    Camera camera;
    CameraProjection proj = CameraProjection::Orthographic;

    ASSERT_NE(camera.getCameraProjection(), proj);

    camera.setCameraProjection(proj);

    ASSERT_EQ(camera.getCameraProjection(), proj);
}

TEST(Camera, SetCameraProjectionMakesCameraCompareNotEqual)
{
    Camera camera;
    Camera copy{camera};
    CameraProjection proj = CameraProjection::Orthographic;

    ASSERT_NE(copy.getCameraProjection(), proj);

    copy.setCameraProjection(proj);

    ASSERT_NE(camera, copy);
}

TEST(Camera, GetPositionReturnsOriginOnDefaultConstruction)
{
    Camera camera;
    ASSERT_EQ(camera.getPosition(), Vec3(0.0f, 0.0f, 0.0f));
}

TEST(Camera, SetDirectionToStandardDirectionCausesGetDirectionToReturnTheDirection)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    Camera camera;

    Vec3 defaultDirection = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.getDirection(), defaultDirection);

    Vec3 differentDirection = Normalize(Vec3{1.0f, 2.0f, -0.5f});
    camera.setDirection(differentDirection);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.getDirection() == differentDirection);  // just ensure it compiles

    camera.setDirection(defaultDirection);

    ASSERT_EQ(camera.getDirection(), defaultDirection);
}

TEST(Camera, SetDirectionToDifferentDirectionGivesAccurateEnoughResults)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    Camera camera;

    Vec3 newDirection = Normalize(Vec3{1.0f, 1.0f, 1.0f});

    camera.setDirection(newDirection);

    Vec3 returnedDirection = camera.getDirection();

    ASSERT_GT(dot(newDirection, returnedDirection), 0.999f);
}

TEST(Camera, GetViewMatrixReturnsViewMatrixBasedOnPositonDirectionAndUp)
{
    Camera camera;
    camera.setCameraProjection(CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    Mat4 viewMatrix = camera.getViewMatrix();
    Mat4 expectedMatrix = Identity<Mat4>();

    ASSERT_EQ(viewMatrix, expectedMatrix);
}

TEST(Camera, SetViewMatrixOverrideSetsANewViewMatrixThatCanBeRetrievedWithGetViewMatrix)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.setCameraProjection(CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});

    Mat4 viewMatrix = Identity<Mat4>();
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrixOverride(viewMatrix);

    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);
}

TEST(Camera, SetViewMatrixOverrideNulloptResetsTheViewMatrixToUsingStandardCameraPositionEtc)
{
    Camera camera;
    Mat4 initialViewMatrix = camera.getViewMatrix();

    Mat4 viewMatrix = Identity<Mat4>();
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrixOverride(viewMatrix);
    ASSERT_NE(camera.getViewMatrix(), initialViewMatrix);
    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);

    camera.setViewMatrixOverride(std::nullopt);

    ASSERT_EQ(camera.getViewMatrix(), initialViewMatrix);
}

TEST(Camera, GetProjectionMatrixReturnsProjectionMatrixBasedOnPositonDirectionAndUp)
{
    Camera camera;
    camera.setCameraProjection(CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    Mat4 mtx = camera.getProjectionMatrix(1.0f);
    Mat4 expected = Identity<Mat4>();

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
    camera.setCameraProjection(CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});

    Mat4 ProjectionMatrix = Identity<Mat4>();
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrixOverride(ProjectionMatrix);

    ASSERT_EQ(camera.getProjectionMatrix(1.0f), ProjectionMatrix);
}

TEST(Camera, SetProjectionMatrixNulloptResetsTheProjectionMatrixToUsingStandardCameraPositionEtc)
{
    Camera camera;
    Mat4 initialProjectionMatrix = camera.getProjectionMatrix(1.0f);

    Mat4 ProjectionMatrix = Identity<Mat4>();
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrixOverride(ProjectionMatrix);
    ASSERT_NE(camera.getProjectionMatrix(1.0f), initialProjectionMatrix);
    ASSERT_EQ(camera.getProjectionMatrix(1.0f), ProjectionMatrix);

    camera.setProjectionMatrixOverride(std::nullopt);

    ASSERT_EQ(camera.getProjectionMatrix(1.0f), initialProjectionMatrix);
}

TEST(Camera, GetViewProjectionMatrixReturnsViewMatrixMultipliedByProjectionMatrix)
{
    Camera camera;

    Mat4 viewMatrix = Identity<Mat4>();
    viewMatrix[0][3] = 2.5f;  // change some part of it

    Mat4 projectionMatrix = Identity<Mat4>();
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    Mat4 expected = projectionMatrix * viewMatrix;

    camera.setViewMatrixOverride(viewMatrix);
    camera.setProjectionMatrixOverride(projectionMatrix);

    ASSERT_EQ(camera.getViewProjectionMatrix(1.0f), expected);
}

TEST(Camera, GetInverseViewProjectionMatrixReturnsExpectedAnswerWhenUsingOverriddenMatrices)
{
    Camera camera;

    Mat4 viewMatrix = Identity<Mat4>();
    viewMatrix[0][3] = 2.5f;  // change some part of it

    Mat4 projectionMatrix = Identity<Mat4>();
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    Mat4 expected = Inverse(projectionMatrix * viewMatrix);

    camera.setViewMatrixOverride(viewMatrix);
    camera.setProjectionMatrixOverride(projectionMatrix);

    ASSERT_EQ(camera.getInverseViewProjectionMatrix(1.0f), expected);
}

TEST(Camera, GetClearFlagsReturnsDefaultOnDefaultConstruction)
{
    Camera camera;
    ASSERT_EQ(camera.getClearFlags(), CameraClearFlags::Default);
}

TEST(Camera, SetClearFlagsCausesGetClearFlagsToReturnNewValue)
{
    Camera camera;

    ASSERT_EQ(camera.getClearFlags(), CameraClearFlags::Default);

    camera.setClearFlags(CameraClearFlags::Nothing);

    ASSERT_EQ(camera.getClearFlags(), CameraClearFlags::Nothing);
}

TEST(Camera, SetClearFlagsCausesCopyToReturnNonEqual)
{
    Camera camera;
    Camera copy{camera};

    ASSERT_EQ(camera, copy);
    ASSERT_EQ(camera.getClearFlags(), CameraClearFlags::Default);

    camera.setClearFlags(CameraClearFlags::Nothing);

    ASSERT_NE(camera, copy);
}
