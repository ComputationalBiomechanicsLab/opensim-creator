#include "polar_camera.hpp"

#include "src/constants.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace {
    glm::mat4 compute_view_matrix(float theta, float phi, float radius, glm::vec3 pan) {
        // camera: at a fixed position pointing at a fixed origin. The "camera"
        // works by translating + rotating all objects around that origin. Rotation
        // is expressed as polar coordinates. Camera panning is represented as a
        // translation vector.

        // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
        // system that shifts the world based on the camera pan

        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
        auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
        auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
        auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
        auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
        return glm::lookAt(glm::vec3(0.0f, 0.0f, radius), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) *
               rot_theta * rot_phi * pan_translate;
    }

    glm::vec3 spherical_2_cartesian(float theta, float phi, float radius) {
        return glm::vec3{radius * sinf(theta) * cosf(phi), radius * sinf(phi), radius * cosf(theta) * cosf(phi)};
    }

    // not currently runtime-editable
    static constexpr float fov = 120.0f;
    static constexpr float znear = 0.1f;
    static constexpr float zfar = 100.0f;
    static constexpr float mouse_wheel_sensitivity = 0.9f;
    static constexpr float mouse_drag_sensitivity = 1.0f;
}

void osmv::Polar_camera::on_scroll_down() noexcept {
    if (radius < 100.0f) {
        radius /= mouse_wheel_sensitivity;
    }
}

void osmv::Polar_camera::on_scroll_up() noexcept {
    if (radius >= 0.1f) {
        radius *= mouse_wheel_sensitivity;
    }
}

void osmv::Polar_camera::on_left_click_down() noexcept {
    is_dragging = true;
}

void osmv::Polar_camera::on_left_click_up() noexcept {
    is_dragging = false;
}

void osmv::Polar_camera::on_right_click_down() noexcept {
    is_panning = true;
}

void osmv::Polar_camera::on_right_click_up() noexcept {
    is_panning = false;
}

void osmv::Polar_camera::on_mouse_motion(float aspect_ratio, float dx, float dy) noexcept {
    if (is_dragging) {
        // alter camera position while dragging
        theta += 2.0f * pi_f * mouse_drag_sensitivity * -dx;
        phi += 2.0f * pi_f * mouse_drag_sensitivity * dy;
    }

    if (is_panning) {
        // how much panning is done depends on how far the camera is from the
        // origin (easy, with polar coordinates) *and* the FoV of the camera.
        float x_amt = dx * aspect_ratio * (2.0f * tanf(fov / 2.0f) * radius);
        float y_amt = -dy * (1.0f / aspect_ratio) * (2.0f * tanf(fov / 2.0f) * radius);

        // this assumes the scene is not rotated, so we need to rotate these
        // axes to match the scene's rotation
        glm::vec4 default_panning_axis = {x_amt, y_amt, 0.0f, 1.0f};
        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
        auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
        auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
        auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), phi, phi_axis);

        glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
        pan.x += panning_axes.x;
        pan.y += panning_axes.y;
        pan.z += panning_axes.z;
    }
}

glm::mat4 osmv::Polar_camera::view_matrix() const noexcept {
    return compute_view_matrix(theta, phi, radius, pan);
}

glm::mat4 osmv::Polar_camera::projection_matrix(float aspect_ratio) const noexcept {
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}

glm::vec3 osmv::Polar_camera::pos() const noexcept {
    return spherical_2_cartesian(theta, phi, radius);
}
