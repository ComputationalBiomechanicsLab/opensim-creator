#include "cameras.hpp"

#include "src/utils/shims.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cmath>

inline constexpr float pi = osc::numbers::pi_v<float>;

void osc::pan(Polar_perspective_camera& cam, float aspect_ratio, glm::vec2 delta) noexcept {
    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float x_amt = delta.x * aspect_ratio * (2.0f * tanf(cam.fov / 2.0f) * cam.radius);
    float y_amt = -delta.y * (1.0f / aspect_ratio) * (2.0f * tanf(cam.fov / 2.0f) * cam.radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    glm::vec4 default_panning_axis = {x_amt, y_amt, 0.0f, 1.0f};
    auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), cam.theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto theta_vec = glm::normalize(glm::vec3{sinf(cam.theta), 0.0f, cosf(cam.theta)});
    auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), cam.phi, phi_axis);

    glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
    cam.pan.x += panning_axes.x;
    cam.pan.y += panning_axes.y;
    cam.pan.z += panning_axes.z;
}

void osc::drag(Polar_perspective_camera& cam, glm::vec2 delta) noexcept {
    cam.theta += 2.0f * pi * -delta.x;
    cam.phi += 2.0f * pi * delta.y;
}

glm::mat4 osc::view_matrix(Polar_perspective_camera const& cam) noexcept {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -cam.theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto theta_vec = glm::normalize(glm::vec3{sinf(cam.theta), 0.0f, cosf(cam.theta)});
    auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -cam.phi, phi_axis);
    auto pan_translate = glm::translate(glm::identity<glm::mat4>(), cam.pan);
    return glm::lookAt(glm::vec3(0.0f, 0.0f, cam.radius), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) *
           rot_theta * rot_phi * pan_translate;
}

glm::mat4 osc::projection_matrix(Polar_perspective_camera const& cam, float aspect_ratio) noexcept {
    return glm::perspective(cam.fov, aspect_ratio, cam.znear, cam.zfar);
}

glm::vec3 osc::pos(Polar_perspective_camera const& cam) noexcept {
    float x = cam.radius * sinf(cam.theta) * cosf(cam.phi);
    float y = cam.radius * sinf(cam.phi);
    float z = cam.radius * cosf(cam.theta) * cosf(cam.phi);

    return glm::vec3{x, y, z};
}

void osc::autoscale_znear_zfar(Polar_perspective_camera& cam) noexcept {
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    cam.znear = 0.02f * cam.radius;
    cam.zfar = 20.0f * cam.radius;
}
