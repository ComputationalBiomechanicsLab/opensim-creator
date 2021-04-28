#pragma once

#include <glm/vec2.hpp>

namespace gl {
    class Texture_2d;
}

namespace osc {
    class Render_target final {
        friend class Renderer;

        struct Impl;
        Impl* impl;

    public:
        Render_target(int w, int h, int samples);
        Render_target(Render_target const&) = delete;
        Render_target(Render_target&&) = delete;
        Render_target& operator=(Render_target const&) = delete;
        Render_target& operator=(Render_target&&) = delete;
        ~Render_target() noexcept;

        [[nodiscard]] Impl& raw_impl() const noexcept;

        void reconfigure(int w, int h, int samples);
        [[nodiscard]] glm::vec2 dimensions() const noexcept;
        [[nodiscard]] int samples() const noexcept;
        [[nodiscard]] float aspect_ratio() const noexcept;

        [[nodiscard]] gl::Texture_2d& main() noexcept;
    };
}
