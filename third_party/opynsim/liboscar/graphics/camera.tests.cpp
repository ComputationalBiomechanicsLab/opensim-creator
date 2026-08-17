#include "camera.h"

#include <liboscar/graphics/camera_projection.h>
#include <liboscar/graphics/clear_flags.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/common_functions.h>
#include <liboscar/maths/constants.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>
#include <liboscar/tests/test_helpers.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <utility>

using namespace osc;
using namespace osc::literals;
using namespace osc::tests;

TEST(Camera, can_default_construct)
{
    const Camera camera;  // should compile + run
}

TEST(Camera, can_copy_construct)
{
    const Camera camera;
    const Camera copy = camera;  // NOLINT(performance-unnecessary-copy-initialization)
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

    c1.set_vertical_field_of_view(1337_deg);

    ASSERT_NE(c1, c2);

    c2.set_vertical_field_of_view(1337_deg);

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

TEST(Camera, projection_defaults_to_Default)
{
    const Camera camera;
    ASSERT_EQ(camera.projection(), CameraProjection::Default);
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

TEST(Camera, vertical_field_of_view_defaults_to_90_deg)
{
    const Camera camera;
    ASSERT_EQ(camera.vertical_field_of_view(), 90_deg);
}

TEST(Camera, set_vertical_field_of_view_sets_the_vertical_field_of_view)
{
    Camera camera;

    ASSERT_EQ(camera.vertical_field_of_view(), 90_deg);
    camera.set_vertical_field_of_view(120_deg);
    ASSERT_EQ(camera.vertical_field_of_view(), 120_deg);
}

TEST(Camera, horizontal_field_of_view_equals_vertical_field_of_view_when_aspect_ratio_is_1)
{
    const Camera camera;
    ASSERT_FLOAT_EQ(camera.vertical_field_of_view().count(), camera.horizontal_field_of_view(1.0f).count());
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
    ASSERT_EQ(camera.position(), Vector3(0.0f, 0.0f, 0.0f));
}

TEST(Camera, set_direction_to_standard_direction_causes_direction_to_return_new_direction)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    Camera camera;

    const Vector3 default_direction = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.direction(), default_direction);

    const Vector3 new_direction = normalize(Vector3{1.0f, 2.0f, -0.5f});
    camera.set_direction(new_direction);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.direction() == new_direction);  // just ensure it compiles

    camera.set_direction(default_direction);

    ASSERT_EQ(camera.direction(), default_direction);
}

TEST(Camera, forward_is_an_alias_to_direction)
{
    Camera camera;
    ASSERT_EQ(camera.direction(), camera.forward());
    camera.set_direction({-1.0f, -1.0f, 0.0f});
    ASSERT_EQ(camera.direction(), camera.forward());
    camera.set_forward({2.0f, 1.0f, 0.5f});
    ASSERT_EQ(camera.forward(), camera.direction());
}

TEST(Camera, set_forward_normalizes_argument)
{
    Camera camera;
    camera.set_direction({1.0f, 2.0f, 3.0f});
    ASSERT_TRUE(all_of(equal_within_absdiff(camera.forward(), normalize(Vector3f{1.0f, 2.0f, 3.0f}), 0.000001f)));
}

TEST(Camera, forward_defaults_to_minus_z)
{
    ASSERT_EQ(Camera{}.forward(), Vector3f(0.0f, 0.0f, -1.0f));
}

TEST(Camera, principal_ray_points_from_origin_along_minus_z_when_default_initialized)
{
    ASSERT_EQ(Camera{}.principal_ray(), Ray(Vector3(0.0f), Vector3(0.0f, 0.0f, -1.0f)));
}

TEST(Camera, principal_ray_is_changed_by_changing_the_position_and_direction_of_the_camera)
{
    Camera camera;
    camera.set_position({3.0f, 2.0f, 1.0f});
    camera.set_direction({1.0f, 0.0f, 0.0f});

    ASSERT_EQ(camera.principal_ray().origin, Vector3(3.0f, 2.0f, 1.0f));
    ASSERT_TRUE(all_of(equal_within_absdiff(camera.principal_ray().direction, Vector3(1.0f, 0.0f, 0.0f), sqrt_epsilon_v<float>)));
}

TEST(Camera, default_rotation_is_identity)
{
    ASSERT_EQ(Camera{}.rotation(), Quaternion{});
}

TEST(Camera, default_position_is_zero)
{
    ASSERT_EQ(Camera{}.position(), Vector3{});
}

TEST(Camera, default_direction_is_minus_z)
{
    ASSERT_EQ(Camera{}.direction(), Vector3(0.0f, 0.0f, -1.0f));
}

TEST(Camera, default_up_is_plus_y)
{
    ASSERT_EQ(Camera{}.up(), Vector3(0.0f, 1.0f, 0.0f));
}

TEST(Camera, set_direction_to_different_direction_gives_accurate_enough_results)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    Camera camera;

    const Vector3 new_direction = normalize(Vector3{1.0f, 1.0f, 1.0f});

    camera.set_direction(new_direction);

    const Vector3 returned_direction = camera.direction();

    ASSERT_GT(dot(new_direction, returned_direction), 0.999f);
}

TEST(Camera, view_matrix_returns_view_matrix_based_on_position_direction_and_up)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({1.0f, 2.0f, 3.0f});

    Matrix4x4 expected_matrix = identity<Matrix4x4>();
    expected_matrix[3][0] = -1.0f;
    expected_matrix[3][1] = -2.0f;
    expected_matrix[3][2] = -3.0f;

    ASSERT_EQ(camera.view_matrix(), expected_matrix);
}

TEST(Camera, inverse_view_matrix_returns_inverse_of_view_matrix_based_on_position_direction_and_up)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({1.0f, 2.0f, 3.0f});

    Matrix4x4 expected_view_matrix = identity<Matrix4x4>();
    expected_view_matrix[3][0] = -1.0f;
    expected_view_matrix[3][1] = -2.0f;
    expected_view_matrix[3][2] = -3.0f;

    ASSERT_EQ(camera.inverse_view_matrix(), inverse(expected_view_matrix));
}

TEST(Camera, set_view_matrix_override_makes_view_matrix_return_the_override)
{
    Camera camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);

    ASSERT_EQ(camera.view_matrix(), view_matrix);
}

TEST(Camera, set_view_matrix_override_to_nullopt_resets_view_matrix_to_use_camera_position_and_up)
{
    Camera camera;
    const Matrix4x4 initial_view_matrix = camera.view_matrix();

    Matrix4x4 view_matrix = identity<Matrix4x4>();
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
    camera.set_clipping_planes({1.0f, -1.0f});

    const Matrix4x4 returned = camera.projection_matrix(1.0f);
    const Matrix4x4 expected = identity<Matrix4x4>();

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

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(projection_matrix);

    ASSERT_EQ(camera.projection_matrix(1.0f), projection_matrix);
}

TEST(Camera, set_projection_matrix_override_to_nullopt_resets_projection_matrix_to_use_camera_field_of_view_etc)
{
    Camera camera;
    const Matrix4x4 initial_projection_matrix = camera.projection_matrix(1.0f);

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
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

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Matrix4x4 expected = projection_matrix * view_matrix;
    ASSERT_EQ(camera.view_projection_matrix(1.0f), expected);
}

TEST(Camera, inverse_view_projection_matrix_returns_expected_matrix)
{
    Camera camera;

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Matrix4x4 expected = inverse(projection_matrix * view_matrix);
    ASSERT_EQ(camera.inverse_view_projection_matrix(1.0f), expected);
}

TEST(Camera, can_call_clipping_planes)
{
    const Camera camera;
    const auto [znear, zfar] = camera.clipping_planes();
    ASSERT_FALSE(isnan(znear));
    ASSERT_FALSE(isnan(zfar));
}

TEST(Camera, clipping_planes_can_be_set_via_set_near_clipping_plane)
{
    Camera camera;
    camera.set_near_clipping_plane(1337.0f);
    ASSERT_EQ(camera.clipping_planes().znear, 1337.0f);
}

TEST(Camera, set_clipping_planes_makes_near_clipping_plane_return_new_near_clipping_plane)
{
    Camera camera;
    camera.set_clipping_planes({-1337.0f, 1337.0f});
    ASSERT_EQ(camera.near_clipping_plane(), -1337.0f);
}

TEST(Camera, set_clipping_planes_makes_far_clipping_plane_return_new_far_clipping_plane)
{
    Camera camera;
    camera.set_clipping_planes({-1337.0f, 1337.0f});
    ASSERT_EQ(camera.far_clipping_plane(), 1337.0f);
}

TEST(Camera, world_to_ui_returns_expected_results_for_orthographic_projection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_orthographic_size(2.0f);
    camera.set_position({2.0f, 100.0f, 2.0f});
    camera.set_forward({0.0f, -1.0f, 0.0f});
    camera.set_up({0.0f, 0.0f, -1.0f});

    const Rect ui_rect = Rect::from_corners({0.0f, 0.0f}, {100.0f, 100.0f});
    const Vector2 got = camera.world_to_ui({1.5f, 2.0f, 1.5f}, ui_rect);
    const Vector2 expected = {25.0f, 25.0f};

    ASSERT_TRUE(all_of(equal_within_absdiff(got, expected, 0.001f)));
}

TEST(Camera, world_to_ui_is_invariant_with_depth_for_orthographic_projection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_orthographic_size(1.0f);
    camera.set_position({1.0f, 1.0f, 0.0f});
    camera.set_forward({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({0.0f, 0.0f}, {64.0f, 64.0f});
    std::optional<Vector2> prev;
    for (size_t i = 1; i < 16; ++i) {
        const Vector2 p = camera.world_to_ui({1.0f, 1.0f, static_cast<float>(i) * 7.5f}, ui_rect);
        ASSERT_TRUE(not prev or all_of(equal_within_absdiff(p, *prev, 0.001f))) << "p = " << p << ", prev = " << prev.value_or(Vector3{-5.0f});
        prev = p;
    }
}

TEST(Camera, world_to_ui_still_works_when_behind_camera_for_orthographic_projection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_orthographic_size(1.0f);
    camera.set_position({1.0f, 1.0f, 0.0f});
    camera.set_forward({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({0.0f, 0.0f}, {64.0f, 64.0f});
    const Vector2 got = camera.world_to_ui({1.0f, 1.0f, -5.0f}, ui_rect);
    const Vector2 expected = {32.0f, 32.0f};

    ASSERT_EQ(got, expected);
}

TEST(Camera, world_to_ui_returns_nan_when_behind_a_perspective_camera)
{
    Camera camera;
    camera.set_projection(CameraProjection::Perspective);
    camera.set_vertical_field_of_view(45_deg);
    camera.set_position({25.0f, 25.0f, 25.0f});
    camera.set_forward({0.0f, 0.0f, -1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({0.0f, 0.0f}, {128.0f, 128.0f});
    const Vector2 got = camera.world_to_ui({25.0f, 25.0f, 30.0f}, ui_rect);

    ASSERT_TRUE(all_of(isnan(got)));
}

TEST(Camera, world_to_ui_returns_center_point_when_given_point_along_principal_ray)
{
    Camera camera;
    camera.set_projection(CameraProjection::Perspective);
    camera.set_vertical_field_of_view(45_deg);
    camera.set_position({25.0f, 25.0f, 25.0f});
    camera.set_forward({0.0f, 0.0f, -1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({0.0f, 0.0f}, {128.0f, 128.0f});
    const Vector2 got = camera.world_to_ui({25.0f, 25.0f, 24.0f}, ui_rect);

    ASSERT_TRUE(all_of(equal_within_absdiff(ui_rect.origin(), got, 0.0001f))) << got;
}

TEST(Camera, ui_to_world_returns_orthogonal_ray_direction_in_orthographic_camera)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_orthographic_size(1.0f);
    camera.set_position({});
    camera.set_forward({0.0f, 0.0f, -1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({}, {64.0f, 64.0f});

    // All rays should be parallel with the camera's direction
    // when given an orthographic camera.
    for (size_t x = 0; x < 8; ++x) {
        for (size_t y = 0; y < 8; ++y) {
            const Vector2f p{x, y};
            const Ray ray = camera.ui_to_world(p, ui_rect);
            ASSERT_EQ(ray.direction, Vector3(0.0f, 0.0f, -1.0f));
        }
    }
}

TEST(Camera, ui_to_world_returns_projected_out_ray_for_projection_camera)
{
    Camera camera;
    camera.set_projection(CameraProjection::Perspective);
    camera.set_vertical_field_of_view(45_deg);
    camera.set_clipping_planes({0.0000001f, 10.0f});
    camera.set_position({});
    camera.set_forward({0.0f, 0.0f, -1.0f});
    camera.set_up({0.0f, 1.0f, 0.0f});

    const Rect ui_rect = Rect::from_corners({}, {64.0f, 64.0f});

    // The point going through the middle should be aligned with
    // the principal ray of the camera (it may have a different origin)
    {
        const Ray center_ray = camera.ui_to_world({32.0f, 32.0f}, ui_rect);
        ASSERT_TRUE(is_colinear_and_codirectional(center_ray, camera.principal_ray()));
    }

    // The point going through the top-middle of the UI rectangle should produce
    // a `Ray` that is aligned with a ray computed from simple triangle geometry:
    //
    //      c  <--- RAY SHOULD SHOOT HERE
    // up  /|
    // ^  / |
    // | /  |  h = znear * tan(vfov/2)
    // |/   |
    // a----b  a->b direction of observation until znear is hit)
    {
        const float znear = camera.near_clipping_plane();
        const Radians vfov = camera.vertical_field_of_view();
        const Vector3 a = camera.position();
        const Vector3 b = a + znear*camera.direction();
        const float h = znear * tan(0.5f*vfov);
        const Vector3 c = b + h*camera.up();
        const Ray expected{camera.position(), normalize(c)};
        const Ray got = camera.ui_to_world({32.0f, 0.0f}, ui_rect);
        ASSERT_TRUE(is_colinear_and_codirectional(expected, got)) << "expected = " << expected << "got = " << got;
    }

    // The point going through the bottom-middle of the UI rectangle should produce
    // a `Ray` that is aligned with a ray computed from simple triangle geometry:
    //
    // up
    // ^
    // |
    // |
    // a----b  a->b direction of observation until znear is hit)
    //  \   |
    //   \  |  h = znear * tan(vfov/2)
    //    \ |
    //     \|
    //      c  <--- RAY SHOULD SHOOT HERE
    {
        const float znear = camera.near_clipping_plane();
        const Radians vfov = camera.vertical_field_of_view();
        const Vector3 a = camera.position();
        const Vector3 b = a + znear*camera.direction();
        const float h = znear * tan(0.5f*vfov);
        const Vector3 c = b + h*-camera.up(); // (go down)
        const Ray expected{camera.position(), normalize(c)};
        const Ray got = camera.ui_to_world({32.0f, 64.0f}, ui_rect);
        ASSERT_TRUE(is_colinear_and_codirectional(expected, got)) << "expected = " << expected << "got = " << got;
    }
}

TEST(Camera, view_volume_height_at_depth_returns_orthographic_height_for_any_depth)
{
    Camera camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_orthographic_size(1.0f);
    camera.set_position({});
    camera.set_forward({0.0f, 1.0f, 0.0f});
    camera.set_up({1.0f, 0.0f, 0.0f});

    for (size_t i = 0; i < 16; ++i) {
        const float h = camera.view_volume_height_at_depth(-8.0f + static_cast<float>(i)*1.0f);
        ASSERT_EQ(h, 1.0f) << "The view volume height should always be the orthographic size with an orthographic camera";
    }
}

TEST(Camera, view_volume_height_at_depth_returns_triangular_values_for_perspective_projection)
{
    Camera camera;
    camera.set_projection(CameraProjection::Perspective);
    camera.set_vertical_field_of_view(45_deg);
    camera.set_position({});
    camera.set_forward({0.0f, 1.0f, 0.0f});

    //            /|
    //           / | <------- h/2 = d*tan(vfov/2)
    //  [0,0,0] P--d [0,d,0]
    //           \ |
    //            \|

    for (size_t i = 0; i < 16; ++i) {
        const float d = static_cast<float>(i) * 0.2f;
        const float got = camera.view_volume_height_at_depth(d);
        const float expected = 2.0f * d * tan(22.5_deg);
        ASSERT_EQ(got, expected);
    }
}
