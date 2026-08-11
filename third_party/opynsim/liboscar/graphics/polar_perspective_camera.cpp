#include "polar_perspective_camera.h"

#include <liboscar/maths/angle.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/trigonometric_functions.h>
#include <liboscar/maths/vector.h>

using namespace osc;
using namespace osc::literals;

namespace
{
    Vector3 PolarToCartesian(Radians theta, Radians phi)
    {
        return {
            sin(theta) * cos(phi),
            sin(phi),
            cos(theta) * cos(phi),
        };
    }

    Vector3 PolarToCartesian(Vector3 focus, float radius, Radians theta, Radians phi)
    {
        return -focus + radius*PolarToCartesian(theta, phi);
    }
}

osc::PolarPerspectiveCamera osc::PolarPerspectiveCamera::with_radius(float radius)
{
    PolarPerspectiveCamera rv;
    rv.radius = radius;
    return rv;
}

osc::PolarPerspectiveCamera osc::PolarPerspectiveCamera::focused_on(const AABB& aabb)
{
    PolarPerspectiveCamera rv;
    rv.focus_on(aabb);
    return rv;
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() = default;

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspect_ratio, Vector2 delta)
{
    const auto horizontal_field_of_view = vertical_to_horizontal_field_of_view(vertical_field_of_view, aspect_ratio);

    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    const float x_amount =  delta.x() * (2.0f * tan(horizontal_field_of_view / 2.0f) * radius);
    const float y_amount = -delta.y() * (2.0f * tan(vertical_field_of_view / 2.0f) * radius);

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

void osc::PolarPerspectiveCamera::drag(Vector2 delta)
{
    theta += 360_deg * -delta.x();
    phi += 360_deg * delta.y();
}

void osc::PolarPerspectiveCamera::rescale_znear_and_zfar_based_on_radius()
{
    // znear and zfar are only really dictated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.1f * radius;
    zfar = 10.0f * radius;
}

Matrix4x4 osc::PolarPerspectiveCamera::view_matrix() const
{
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's
    // a polar coordinate system that shifts the world based on the camera pan

    const Matrix4x4 theta_rotation = rotate(identity<Matrix4x4>(), -theta, Vector3{0.0f, 1.0f, 0.0f});
    const Vector3 theta_vec = normalize(Vector3{sin(theta), 0.0f, cos(theta)});
    const Vector3 phi_axis = cross(theta_vec, Vector3{0.0, 1.0f, 0.0f});
    const Matrix4x4 phi_rotation = rotate(identity<Matrix4x4>(), -phi, phi_axis);
    const Matrix4x4 pan_translation = translate(identity<Matrix4x4>(), focus_point);
    return look_at(
        Vector3(0.0f, 0.0f, radius),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3{0.0f, 1.0f, 0.0f}) * theta_rotation * phi_rotation * pan_translation;
}

Matrix4x4 osc::PolarPerspectiveCamera::projection_matrix(float aspect_ratio) const
{
    return perspective(vertical_field_of_view, aspect_ratio, znear, zfar);
}

Vector3 osc::PolarPerspectiveCamera::position() const
{
    return PolarToCartesian(focus_point, radius, theta, phi);
}

Vector3 PolarPerspectiveCamera::forward() const
{
    return -PolarToCartesian(theta, phi);
}

CameraClippingPlanes osc::PolarPerspectiveCamera::clipping_planes() const
{
    return {znear, zfar};
}

Vector2 osc::PolarPerspectiveCamera::project_onto_viewport(
    const Vector3& world_space_position,
    const Rect& viewport_rect) const
{
    return osc::project_onto_viewport_rect(
        world_space_position,
        view_matrix(),
        projection_matrix(aspect_ratio_of(viewport_rect)),
        viewport_rect
    );
}

Ray osc::PolarPerspectiveCamera::unproject_topleft_position_to_world_ray(Vector2 position, Vector2 dimensions) const
{
    return perspective_unproject_topleft_normalized_pos_to_world(
        position / dimensions,
        this->position(),
        view_matrix(),
        projection_matrix(aspect_ratio_of(dimensions))
    );
}

float osc::PolarPerspectiveCamera::frustum_height_at_depth(float depth) const
{
    return 2.0f * depth * tan(0.5f * vertical_field_of_view);
}

void osc::PolarPerspectiveCamera::focus_along(CoordinateDirection coordinate_direction)
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

void osc::PolarPerspectiveCamera::focus_on(
    const AABB& aabb,
    float aspect_ratio)
{
    const Sphere bounding_sphere = bounding_sphere_of(aabb);
    const Radians smallest_fov = aspect_ratio >= 1.0f ?
        vertical_field_of_view :
        vertical_to_horizontal_field_of_view(vertical_field_of_view, aspect_ratio);

    // auto-focus the camera with a minimum radius of 1m
    //
    // this will break autofocusing on very small models (e.g. insect legs) but
    // handles the edge-case of autofocusing an empty model (opensim-creator#552), which is a
    // more common use-case (e.g. for new users and users making human-sized models)
    focus_point = -bounding_sphere.origin;
    radius = max(bounding_sphere.radius / tan(smallest_fov/2.0), 1.0f);
    rescale_znear_and_zfar_based_on_radius();
}
