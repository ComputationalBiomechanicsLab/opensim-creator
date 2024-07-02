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

TEST(Camera, can_default_construct)
{
    const Camera camera;  // should compile + run
}

TEST(Camera, can_copy_construct)
{
    const Camera camera;
    const Camera copy = camera;
}

TEST(Camera, copied_instance_compares_equal_to_original)
{
    const Camera camera;
    const Camera copy = camera;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(camera, copy);
}

TEST(Camera, can_move_construct)
{
    Camera camera;
    const Camera copy{std::move(camera)};
}

TEST(Camera, can_copy_assign)
{
    const Camera c1;
    Camera c2;

    c2 = c1;
}

TEST(Camera, copy_assigned_instance_compares_equal_to_rhs)
{
    Camera c1;
    const Camera c2;

    c1 = c2;

    ASSERT_EQ(c1, c2);
}

TEST(Camera, can_move_assign)
{
    Camera c1;
    Camera c2;

    c2 = std::move(c1);
}

TEST(Camera, uses_value_comparision)
{
    Camera c1;
    Camera c2;

    ASSERT_EQ(c1, c2);

    c1.set_vertical_fov(1337_deg);

    ASSERT_NE(c1, c2);

    c2.set_vertical_fov(1337_deg);

    ASSERT_EQ(c1, c2);
}

TEST(Camera, reset_resets_the_instance_to_default_values)
{
    const Camera default_camera;
    Camera camera = default_camera;
    camera.set_direction({1.0f, 0.0f, 0.0f});
    ASSERT_NE(camera, default_camera);
    camera.reset();
    ASSERT_EQ(camera, default_camera);
}

TEST(Camera, background_color_returns_clear_if_not_set)
{
    const Camera camera;
    ASSERT_EQ(camera.background_color(), Color::clear());
}

TEST(Camera, can_call_set_background_color)
{
    Camera camera;
    camera.set_background_color(generate<Color>());
}

TEST(Camera, set_background_color_makes_getter_return_new_color)
{
    Camera camera;
    const Color color = generate<Color>();

    camera.set_background_color(color);

    ASSERT_EQ(camera.background_color(), color);
}

TEST(Camera, set_background_color_on_copy_makes_camera_compare_non_equal_with_copy_source)
{
    const Camera camera;
    Camera copy{camera};

    ASSERT_EQ(camera, copy);

    copy.set_background_color(generate<Color>());

    ASSERT_NE(camera, copy);
}

TEST(Camera, clear_flags_defaults_to_SolidColor_and_Depth)
{
    const Camera camera;

    ASSERT_TRUE(camera.clear_flags() & CameraClearFlags::SolidColor);
    ASSERT_TRUE(camera.clear_flags() & CameraClearFlags::Depth);
}

TEST(Camera, set_clear_flags_works_as_expected)
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

TEST(Camera, projection_defaults_to_Perspective)
{
    const Camera camera;
    ASSERT_EQ(camera.projection(), CameraProjection::Perspective);
}

TEST(Camera, can_call_set_projection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
}

TEST(Camera, set_projection_makes_getter_return_the_projection)
{
    Camera camera;
    const CameraProjection new_projection = CameraProjection::Orthographic;

    ASSERT_NE(camera.projection(), new_projection);
    camera.set_projection(new_projection);
    ASSERT_EQ(camera.projection(), new_projection);
}

TEST(Camera, set_projection_on_copy_makes_it_compare_nonequal_to_original)
{
    const Camera camera;
    Camera copy = camera;
    const CameraProjection new_projection = CameraProjection::Orthographic;

    ASSERT_NE(copy.projection(), new_projection);
    copy.set_projection(new_projection);
    ASSERT_NE(camera, copy);
}

TEST(Camera, position_defaults_to_zero_vector)
{
    const Camera camera;
    ASSERT_EQ(camera.position(), Vec3(0.0f, 0.0f, 0.0f));
}

TEST(Camera, set_direction_to_standard_direction_causes_direction_to_return_new_direction)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    Camera camera;

    const Vec3 default_direction = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.direction(), default_direction);

    Vec3 new_direction = normalize(Vec3{1.0f, 2.0f, -0.5f});
    camera.set_direction(new_direction);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.direction() == new_direction);  // just ensure it compiles

    camera.set_direction(default_direction);

    ASSERT_EQ(camera.direction(), default_direction);
}

TEST(Camera, set_direction_to_different_direction_gives_accurate_enough_results)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    Camera camera;

    Vec3 new_direction = normalize(Vec3{1.0f, 1.0f, 1.0f});

    camera.set_direction(new_direction);

    Vec3 returned_direction = camera.direction();

    ASSERT_GT(dot(new_direction, returned_direction), 0.999f);
}

TEST(Camera, view_matrix_returns_view_matrix_based_on_position_direction_and_up)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({0.0f, 0.0f, 0.0f});

    const Mat4 view_matrix = camera.view_matrix();
    const Mat4 expected_matrix = identity<Mat4>();

    ASSERT_EQ(view_matrix, expected_matrix);
}

TEST(Camera, set_view_matrix_override_makes_view_matrix_return_the_override)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Mat4 view_matrix = identity<Mat4>();
    view_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);

    ASSERT_EQ(camera.view_matrix(), view_matrix);
}

TEST(Camera, set_view_matrix_override_to_nullopt_resets_view_matrix_to_use_camera_position_and_up)
{
    Camera camera;
    Mat4 initial_view_matrix = camera.view_matrix();

    Mat4 view_matrix = identity<Mat4>();
    view_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    ASSERT_NE(camera.view_matrix(), initial_view_matrix);
    ASSERT_EQ(camera.view_matrix(), view_matrix);

    camera.set_view_matrix_override(std::nullopt);

    ASSERT_EQ(camera.view_matrix(), initial_view_matrix);
}

TEST(Camera, projection_matrix_returns_matrix_based_on_camera_position_and_up)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({0.0f, 0.0f, 0.0f});

    const Mat4 returned = camera.projection_matrix(1.0f);
    const Mat4 expected = identity<Mat4>();

    // only compare the Y, Z, and W columns: the X column depends on the aspect ratio of the output
    // target
    ASSERT_EQ(returned[1], expected[1]);
    ASSERT_EQ(returned[2], expected[2]);
    ASSERT_EQ(returned[3], expected[3]);
}

TEST(Camera, set_projection_matrix_override_makes_projection_matrix_return_the_override)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Mat4 projection_matrix = identity<Mat4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(projection_matrix);

    ASSERT_EQ(camera.projection_matrix(1.0f), projection_matrix);
}

TEST(Camera, set_projection_matrix_override_to_nullopt_resets_projection_matrix_to_use_camera_fov_etc)
{
    Camera camera;
    const Mat4 initial_projection_matrix = camera.projection_matrix(1.0f);

    Mat4 projection_matrix = identity<Mat4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(projection_matrix);
    ASSERT_NE(camera.projection_matrix(1.0f), initial_projection_matrix);
    ASSERT_EQ(camera.projection_matrix(1.0f), projection_matrix);

    camera.set_projection_matrix_override(std::nullopt);

    ASSERT_EQ(camera.projection_matrix(1.0f), initial_projection_matrix);
}

TEST(Camera, view_projection_matrix_returns_view_matrix_multiplied_by_projection_matrix)
{
    Camera camera;

    Mat4 view_matrix = identity<Mat4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Mat4 projection_matrix = identity<Mat4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Mat4 expected = projection_matrix * view_matrix;
    ASSERT_EQ(camera.view_projection_matrix(1.0f), expected);
}

TEST(Camera, inverse_view_projection_matrix_returns_expected_matrix)
{
    Camera camera;

    Mat4 view_matrix = identity<Mat4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Mat4 projection_matrix = identity<Mat4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Mat4 expected = inverse(projection_matrix * view_matrix);
    ASSERT_EQ(camera.inverse_view_projection_matrix(1.0f), expected);
}

TEST(Camera, clear_flags_defaults_to_Default)
{
    Camera camera;
    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);
}

TEST(Camera, set_clear_flags_causes_clear_flags_to_return_new_flags)
{
    Camera camera;

    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);
    camera.set_clear_flags(CameraClearFlags::Nothing);
    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Nothing);
}

TEST(Camera, set_clear_flags_causes_copy_to_compare_not_equivalent)
{
    Camera camera;
    Camera copy = camera;

    ASSERT_EQ(camera, copy);
    ASSERT_EQ(camera.clear_flags(), CameraClearFlags::Default);
    camera.set_clear_flags(CameraClearFlags::Nothing);
    ASSERT_NE(camera, copy);
}
