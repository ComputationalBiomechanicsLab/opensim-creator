#include "orbit_camera_controller.h"

#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/math_helpers.h>

using namespace osc;
using namespace osc::literals;

osc::OrbitCameraController osc::OrbitCameraController::focused_on(
    const AABB& aabb,
    const Camera& camera,
    float aspect_ratio)
{
    OrbitCameraController rv;
    rv.focus_on(aabb, camera, aspect_ratio);
    return rv;
}

void osc::OrbitCameraController::update_camera(Camera& camera) const
{
    const float sin_theta = sin(theta);
    const float sin_phi = sin(phi);
    const float cos_theta = cos(theta);
    const float cos_phi = cos(phi);
    const Vector3 back = { sin_theta*cos_phi, sin_phi,  cos_theta*cos_phi};
    const Vector3 up =   {-sin_theta*sin_phi, cos_phi, -cos_theta*sin_phi};

    camera.set_position(focus_point + radius*back);
    camera.set_up(up);
    camera.set_direction(-back);
    camera.set_clipping_planes({0.1f * radius, 10.0f * radius});
}
void osc::OrbitCameraController::reset()
{
    *this = {};
}

void osc::OrbitCameraController::pan(
    float aspect_ratio,
    Vector2 normalized_delta,
    const Camera& camera)
{
    // TODO: fix this with Orthographic projections
    const Radians vertical_field_of_view = camera.vertical_field_of_view();
    const auto horizontal_field_of_view = vertical_to_horizontal_field_of_view(vertical_field_of_view, aspect_ratio);

    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    const float x_amount =  normalized_delta.x() * (2.0f * tan(horizontal_field_of_view / 2.0f) * radius);
    const float y_amount = -normalized_delta.y() * (2.0f * tan(vertical_field_of_view / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    const Vector4 default_panning_axis = {x_amount, y_amount, 0.0f, 1.0f};
    const Matrix4x4 rotation_theta = rotate(identity<Matrix4x4>(), theta, Vector3{0.0f, 1.0f, 0.0f});
    const Vector3 theta_vec{sin(theta), 0.0f, cos(theta)};
    const Vector3 phi_axis = cross(theta_vec, Vector3{0.0f, 1.0f, 0.0f});
    const Matrix4x4 rotation_phi = rotate(identity<Matrix4x4>(), phi, phi_axis);

    const Vector4 panning_axes = rotation_phi * rotation_theta * default_panning_axis;
    focus_point += Vector3{panning_axes};
}

void osc::OrbitCameraController::drag(Vector2 normalized_delta)
{
    theta += 360_deg * -normalized_delta.x();
    phi += 360_deg * normalized_delta.y();
}

void osc::OrbitCameraController::focus_on(
    const AABB& aabb,
    const Camera& camera,
    float aspect_ratio)
{
    // Always focus on the centroid of the AABB
    focus_point = centroid_of(aabb);

    // If the camera is a perspective camera, adjust the camera's radius
    // to try and fit the entire aabb in-frame.
    if (camera.projection() == CameraProjection::Perspective) {
        const Sphere bounding_sphere = bounding_sphere_of(aabb);
        const Radians vfov = camera.vertical_field_of_view();
        const Radians smallest_fov = aspect_ratio >= 1.0f ?
            vfov :
            vertical_to_horizontal_field_of_view(vfov, aspect_ratio);

        // use a minimum radius of 1m
        //
        // this will break autofocusing on very small models (e.g. insect legs) but
        // handles the edge-case of autofocusing an empty model (opensim-creator#552), which is a
        // more common use-case (e.g. for new users and users making human-sized models)
        radius = max(bounding_sphere.radius / tan(smallest_fov/2.0), 1.0f);
    }
}

void osc::OrbitCameraController::focus_along(CoordinateDirection coordinate_direction)
{
    switch (coordinate_direction.index()) {
    case CoordinateDirection::x().index():       theta =  90_deg;  phi =  0_deg;  break;
    case CoordinateDirection::y().index():       theta =  0_deg;   phi =  90_deg; break;
    case CoordinateDirection::z().index():       theta =  0_deg;   phi =  0_deg;  break;
    case CoordinateDirection::minus_x().index(): theta = -90_deg;  phi =  0_deg;  break;
    case CoordinateDirection::minus_y().index(): theta =  0_deg;   phi = -90_deg; break;
    case CoordinateDirection::minus_z().index(): theta =  180_deg; phi =  0_deg;  break;
    default:                                                                      break;
    }
}

void osc::OrbitCameraController::focus_along(const Vector3& direction)
{
    theta = atan2(-direction.x(), -direction.z());
    phi = asin(-direction.y());
}