#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace osmv {
    class Polar_camera final {
    public:
        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        bool is_dragging = false;
        bool is_panning = false;

        Polar_camera() = default;

        void on_scroll_down() noexcept;
        void on_scroll_up() noexcept;

        void on_left_click_down() noexcept;
        void on_left_click_up() noexcept;

        void on_right_click_down() noexcept;
        void on_right_click_up() noexcept;

        void on_mouse_motion(float aspect_ratio, float dx, float dy) noexcept;

        glm::mat4 view_matrix() const noexcept;
        glm::mat4 projection_matrix(float aspect_ratio) const noexcept;
        glm::vec3 pos() const noexcept;
    };
}
