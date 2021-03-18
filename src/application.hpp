#pragma once

#include "src/assertions.hpp"
#include "src/utils/geometry.hpp"

#include <memory>
#include <utility>

namespace osmv {
    class Screen;
}

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance
namespace osmv {
    class Application final {
        static Application* gCurrent;

        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        static void set_current(Application* ptr) noexcept {
            gCurrent = ptr;
        }

        [[nodiscard]] static Application& current() noexcept {
            OSMV_ASSERT(gCurrent != nullptr);
            return *gCurrent;
        }

        Application();
        Application(Application const&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application const&) = delete;
        Application& operator=(Application&&) = delete;
        ~Application() noexcept;

        void start_render_loop(std::unique_ptr<Screen>);

        template<typename T, typename... Args>
        void start_render_loop(Args&&... args) {
            start_render_loop(std::make_unique<T>(std::forward<Args>(args)...));
        }

        void request_screen_transition(std::unique_ptr<osmv::Screen>);

        template<typename Screen, typename... Args>
        void request_screen_transition(Args&&... args) {
            request_screen_transition(std::make_unique<Screen>(std::forward<Args>(args)...));
        }

        void request_quit_application();

        [[nodiscard]] Dimensions<int> window_dimensions() const noexcept;

        // returns the number of samples (MSXAA) that multisampled renderers should use
        [[nodiscard]] int samples() const noexcept;

        // returns the maximum number of samples (MSXAA) that the OpenGL backend supports
        [[nodiscard]] int max_samples() const noexcept;

        // set the number of samples (MSXAA) that multisampled renderers should use
        void set_samples(int);

        // returns true if the application is rendering in debug mode (i.e. whether
        // downstream rendererers should also render debug info)
        [[nodiscard]] bool is_in_debug_mode() const noexcept;

        // makes the application window fullscreen
        void make_fullscreen();

        // makes the application window windowed (as opposed to fullscreen)
        void make_windowed();

        [[nodiscard]] bool is_vsync_enabled() const noexcept;

        void enable_vsync();

        void disable_vsync();

        [[nodiscard]] bool is_in_opengl_debug_mode() const noexcept;
        void enable_opengl_debug_mode();
        void disable_opengl_debug_mode();
    };
}
