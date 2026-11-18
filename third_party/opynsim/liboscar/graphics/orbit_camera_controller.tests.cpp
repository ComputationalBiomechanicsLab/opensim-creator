#include "orbit_camera_controller.h"

#include <liboscar/graphics/camera.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/constants.h>
#include <liboscar/maths/coordinate_direction.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/tests/test_helpers.h>

#include <gtest/gtest.h>

using namespace osc;
using namespace osc::literals;

namespace
{
    /// Returns the error tolerance for `controller`'s position.
    float position_tolerance(const OrbitCameraController& controller)
    {
        return 10.0f * epsilon_v<float> * max(controller.radius, 1.0f);
    }
}

TEST(OrbitCameraController, radius_defaults_to_1)
{
    ASSERT_EQ(OrbitCameraController{}.radius, 1.0f);
}

TEST(OrbitCameraController, focus_point_defaults_to_zero)
{
    ASSERT_EQ(OrbitCameraController{}.focus_point, Vector3{});
}

TEST(OrbitCameraController, position_takes_radius_into_account)
{
    for (const float& radius : {0.0f, 1.0f, 3.5f, 10.0f}) {
        // X
        {
            OrbitCameraController controller;
            controller.radius = radius;
            controller.focus_along(CoordinateDirection::x());
            OSC_ASSERT_VECTOR_NEAR(
                controller.position(), Vector3(radius, 0.0f, 0.0f), position_tolerance(controller));
        }

        // -Z
        {
            OrbitCameraController controller;
            controller.radius = radius;
            controller.focus_along(CoordinateDirection::minus_z());
            OSC_ASSERT_VECTOR_NEAR(
                controller.position(),
                Vector3(0.0f, 0.0f, -radius),
                position_tolerance(controller));
        }
    }
}

TEST(OrbitCameraController, set_position_updates_position_and_polar_coordinates)
{
    OrbitCameraController controller;
    controller.set_position({0.0f, 3.0f, 4.0f});

    OSC_ASSERT_VECTOR_NEAR(
        controller.position(), Vector3(0.0f, 3.0f, 4.0f), 10.0f * epsilon_v<float>);
    ASSERT_NEAR(controller.radius, length(Vector3{0.0f, 3.0f, 4.0f}), sqrt_epsilon_v<float>);
    OSC_ASSERT_ANGLE_NEAR(controller.theta, 0_deg, Radians(10.0f * epsilon_v<float>));
}

TEST(OrbitCameraController, set_position_respects_custom_focus_point)
{
    OrbitCameraController controller;
    controller.focus_point = {1.0f, 1.0f, 1.0f};
    controller.set_position({1.0f, 6.0f, 1.0f});  // Directly above

    OSC_ASSERT_VECTOR_NEAR(controller.position(), Vector3(1.0f, 6.0f, 1.0f), sqrt_epsilon_v<float>);
    ASSERT_NEAR(controller.radius, 5.0f, sqrt_epsilon_v<float>);
    OSC_ASSERT_ANGLE_NEAR(controller.phi, 90_deg, Radians(10.0f * epsilon_v<float>));
}

TEST(OrbitCameraController, set_position_at_focus_point_sets_radius_to_zero_safely)
{
    OrbitCameraController controller;
    const auto initial_theta = controller.theta;
    const auto initial_phi = controller.phi;

    controller.focus_point = {2.0f, 3.0f, 4.0f};
    controller.set_position(controller.focus_point);

    ASSERT_EQ(controller.radius, 0.0f);
    OSC_ASSERT_VECTOR_NEAR(controller.position(), controller.focus_point, 10.0f * epsilon_v<float>);
    ASSERT_EQ(controller.theta, initial_theta)
        << "theta should be preserved when positioning at zero";
    ASSERT_EQ(controller.phi, initial_phi) << "phi should be preserved when positioning at zero";
}

TEST(OrbitCameraController, direction_works_as_expected_when_zeroing_theta_and_phi)
{
    OrbitCameraController controller;
    controller.theta = {};
    controller.phi = {};

    OSC_ASSERT_VECTOR_NEAR(
        controller.direction(), Vector3(0.0f, 0.0f, -1.0f), 10.0f * epsilon_v<float>);
}

TEST(OrbitCameraController, direction_works_as_expected_when_looking_down_a_coordinate_axis)
{
    OrbitCameraController controller;
    controller.focus_along(CoordinateDirection::y());

    OSC_ASSERT_VECTOR_NEAR(
        controller.direction(), Vector3(0.0f, -1.0f, 0.0f), 10.0f * epsilon_v<float>);
}

TEST(OrbitCameraController, set_position_and_direction_from_works_as_expected)
{
    const Camera camera{
        {-1.0f, 3.0f, -5.0f},  // position
        {+0.0f, 0.0f, -1.0f},  // direction
        {+0.0f, 1.0f, +0.0f},  // up
    };
    OrbitCameraController controller;
    controller.set_position_and_direction_from(camera);
    OSC_ASSERT_VECTOR_NEAR(
        controller.position(), Vector3(-1.0f, 3.0f, -5.0f), 10.0f * epsilon_v<float>);
    OSC_ASSERT_VECTOR_NEAR(
        controller.direction(), Vector3(0.0f, 0.0f, -1.0f), 10.0f * epsilon_v<float>);
    ASSERT_EQ(controller.radius, 1.0f);
}
