#include "camera_v2.h"

#include <liboscar/graphics/camera_clear_flags.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/tests/test_helpers.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <utility>

using namespace osc;
using namespace osc::literals;
using namespace osc::tests;

TEST(CameraV2, can_default_construct)
{
    const CameraV2 camera;  // should compile + run
}

TEST(CameraV2, can_copy_construct)
{
    const CameraV2 camera;
    const CameraV2 copy = camera;  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST(CameraV2, copied_instance_compares_equal_to_original)
{
    const CameraV2 camera;
    const CameraV2 copy = camera;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(camera, copy);
}

TEST(CameraV2, can_move_construct)
{
    CameraV2 camera;
    const CameraV2 copy{std::move(camera)};
}

TEST(CameraV2, can_copy_assign)
{
    const CameraV2 c1;
    CameraV2 c2;

    c2 = c1;
}

TEST(CameraV2, copy_assigned_instance_compares_equal_to_rhs)
{
    CameraV2 c1;
    const CameraV2 c2;

    c1 = c2;

    ASSERT_EQ(c1, c2);
}

TEST(CameraV2, can_move_assign)
{
    CameraV2 c1;
    CameraV2 c2;

    c2 = std::move(c1);
}

TEST(CameraV2, uses_value_comparision)
{
    CameraV2 c1;
    CameraV2 c2;

    ASSERT_EQ(c1, c2);

    c1.set_vertical_field_of_view(1337_deg);

    ASSERT_NE(c1, c2);

    c2.set_vertical_field_of_view(1337_deg);

    ASSERT_EQ(c1, c2);
}

TEST(CameraV2, reset_resets_the_instance_to_default_values)
{
    const CameraV2 default_camera;
    CameraV2 camera = default_camera;
    camera.set_direction({1.0f, 0.0f, 0.0f});
    ASSERT_NE(camera, default_camera);
    camera.reset();
    ASSERT_EQ(camera, default_camera);
}

TEST(CameraV2, projection_defaults_to_Default)
{
    const CameraV2 camera;
    ASSERT_EQ(camera.projection(), CameraProjection::Default);
}

TEST(CameraV2, can_call_set_projection)
{
    CameraV2 camera;
    camera.set_projection(CameraProjection::Orthographic);
}

TEST(CameraV2, set_projection_makes_getter_return_the_projection)
{
    CameraV2 camera;
    const CameraProjection new_projection = CameraProjection::Orthographic;

    ASSERT_NE(camera.projection(), new_projection);
    camera.set_projection(new_projection);
    ASSERT_EQ(camera.projection(), new_projection);
}

TEST(CameraV2, vertical_field_of_view_defaults_to_90_deg)
{
    const CameraV2 camera;
    ASSERT_EQ(camera.vertical_field_of_view(), 90_deg);
}

TEST(CameraV2, set_vertical_field_of_view_sets_the_vertical_field_of_view)
{
    CameraV2 camera;

    ASSERT_EQ(camera.vertical_field_of_view(), 90_deg);
    camera.set_vertical_field_of_view(120_deg);
    ASSERT_EQ(camera.vertical_field_of_view(), 120_deg);
}

TEST(CameraV2, horizontal_field_of_view_equals_vertical_field_of_view_when_aspect_ratio_is_1)
{
    const CameraV2 camera;
    ASSERT_FLOAT_EQ(camera.vertical_field_of_view().count(), camera.horizontal_field_of_view(1.0f).count());
}

TEST(CameraV2, set_projection_on_copy_makes_it_compare_nonequal_to_original)
{
    const CameraV2 camera;
    CameraV2 copy = camera;
    const CameraProjection new_projection = CameraProjection::Orthographic;

    ASSERT_NE(copy.projection(), new_projection);
    copy.set_projection(new_projection);
    ASSERT_NE(camera, copy);
}

TEST(CameraV2, position_defaults_to_zero_vector)
{
    const CameraV2 camera;
    ASSERT_EQ(camera.position(), Vector3(0.0f, 0.0f, 0.0f));
}

TEST(CameraV2, set_direction_to_standard_direction_causes_direction_to_return_new_direction)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    CameraV2 camera;

    const Vector3 default_direction = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.direction(), default_direction);

    const Vector3 new_direction = normalize(Vector3{1.0f, 2.0f, -0.5f});
    camera.set_direction(new_direction);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.direction() == new_direction);  // just ensure it compiles

    camera.set_direction(default_direction);

    ASSERT_EQ(camera.direction(), default_direction);
}

TEST(CameraV2, set_direction_to_different_direction_gives_accurate_enough_results)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    CameraV2 camera;

    const Vector3 new_direction = normalize(Vector3{1.0f, 1.0f, 1.0f});

    camera.set_direction(new_direction);

    const Vector3 returned_direction = camera.direction();

    ASSERT_GT(dot(new_direction, returned_direction), 0.999f);
}

TEST(CameraV2, view_matrix_returns_view_matrix_based_on_position_direction_and_up)
{
    CameraV2 camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({1.0f, 2.0f, 3.0f});

    Matrix4x4 expected_matrix = identity<Matrix4x4>();
    expected_matrix[3][0] = -1.0f;
    expected_matrix[3][1] = -2.0f;
    expected_matrix[3][2] = -3.0f;

    ASSERT_EQ(camera.view_matrix(), expected_matrix);
}

TEST(CameraV2, inverse_view_matrix_returns_inverse_of_view_matrix_based_on_position_direction_and_up)
{
    CameraV2 camera;
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({1.0f, 2.0f, 3.0f});

    Matrix4x4 expected_view_matrix = identity<Matrix4x4>();
    expected_view_matrix[3][0] = -1.0f;
    expected_view_matrix[3][1] = -2.0f;
    expected_view_matrix[3][2] = -3.0f;

    ASSERT_EQ(camera.inverse_view_matrix(), inverse(expected_view_matrix));
}

TEST(CameraV2, set_view_matrix_override_makes_view_matrix_return_the_override)
{
    CameraV2 camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);

    ASSERT_EQ(camera.view_matrix(), view_matrix);
}

TEST(CameraV2, set_view_matrix_override_to_nullopt_resets_view_matrix_to_use_camera_position_and_up)
{
    CameraV2 camera;
    const Matrix4x4 initial_view_matrix = camera.view_matrix();

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    ASSERT_NE(camera.view_matrix(), initial_view_matrix);
    ASSERT_EQ(camera.view_matrix(), view_matrix);

    camera.set_view_matrix_override(std::nullopt);

    ASSERT_EQ(camera.view_matrix(), initial_view_matrix);
}

TEST(CameraV2, projection_matrix_returns_matrix_based_on_camera_position_and_up)
{
    CameraV2 camera;
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

TEST(CameraV2, set_projection_matrix_override_makes_projection_matrix_return_the_override)
{
    CameraV2 camera;

    // these shouldn't matter - they're overridden
    camera.set_projection(CameraProjection::Orthographic);
    camera.set_position({7.0f, 5.0f, -3.0f});

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(projection_matrix);

    ASSERT_EQ(camera.projection_matrix(1.0f), projection_matrix);
}

TEST(CameraV2, set_projection_matrix_override_to_nullopt_resets_projection_matrix_to_use_camera_field_of_view_etc)
{
    CameraV2 camera;
    const Matrix4x4 initial_projection_matrix = camera.projection_matrix(1.0f);

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_projection_matrix_override(projection_matrix);
    ASSERT_NE(camera.projection_matrix(1.0f), initial_projection_matrix);
    ASSERT_EQ(camera.projection_matrix(1.0f), projection_matrix);

    camera.set_projection_matrix_override(std::nullopt);

    ASSERT_EQ(camera.projection_matrix(1.0f), initial_projection_matrix);
}

TEST(CameraV2, view_projection_matrix_returns_view_matrix_multiplied_by_projection_matrix)
{
    CameraV2 camera;

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Matrix4x4 expected = projection_matrix * view_matrix;
    ASSERT_EQ(camera.view_projection_matrix(1.0f), expected);
}

TEST(CameraV2, inverse_view_projection_matrix_returns_expected_matrix)
{
    CameraV2 camera;

    Matrix4x4 view_matrix = identity<Matrix4x4>();
    view_matrix[0][3] = 2.5f;  // change some part of it

    Matrix4x4 projection_matrix = identity<Matrix4x4>();
    projection_matrix[0][1] = 9.0f;  // change some part of it

    camera.set_view_matrix_override(view_matrix);
    camera.set_projection_matrix_override(projection_matrix);

    const Matrix4x4 expected = inverse(projection_matrix * view_matrix);
    ASSERT_EQ(camera.inverse_view_projection_matrix(1.0f), expected);
}

TEST(CameraV2, can_call_clipping_planes)
{
    const CameraV2 camera;
    const auto [znear, zfar] = camera.clipping_planes();
    ASSERT_FALSE(isnan(znear));
    ASSERT_FALSE(isnan(zfar));
}

TEST(CameraV2, clipping_planes_can_be_set_via_set_near_clipping_plane)
{
    CameraV2 camera;
    camera.set_near_clipping_plane(1337.0f);
    ASSERT_EQ(camera.clipping_planes().znear, 1337.0f);
}

TEST(CameraV2, set_clipping_planes_makes_near_clipping_plane_return_new_near_clipping_plane)
{
    CameraV2 camera;
    camera.set_clipping_planes({-1337.0f, 1337.0f});
    ASSERT_EQ(camera.near_clipping_plane(), -1337.0f);
}

TEST(CameraV2, set_clipping_planes_makes_far_clipping_plane_return_new_far_clipping_plane)
{
    CameraV2 camera;
    camera.set_clipping_planes({-1337.0f, 1337.0f});
    ASSERT_EQ(camera.far_clipping_plane(), 1337.0f);
}
