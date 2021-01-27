#pragma once

#include "sdl_wrapper.hpp"

#include <memory>

namespace gl {
    class Render_buffer;
    class Frame_buffer;
}

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance
namespace osmv {
    class Screen;
    struct Application_impl;

    class Application final {
        std::unique_ptr<Application_impl> impl;

    public:
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

        template<typename T, typename... Args>
        void request_screen_transition(Args&&... args) {
            request_screen_transition(std::make_unique<T>(std::forward<Args>(args)...));
        }

        void request_quit_application();

        // true if FPS is being throttled (e.g. with software (sleeps) or vsync)
        bool is_throttling_fps() const noexcept;
        void is_throttling_fps(bool);

        // dimensions of the main application window in pixels
        sdl::Window_dimensions window_dimensions() const noexcept;

        float window_aspect_ratio() const noexcept {
            auto [w, h] = window_dimensions();
            return static_cast<float>(w) / static_cast<float>(h);
        }

        // move mouse relative to the window (origin in top-left)
        void move_mouse_to(int x, int y);

        // returns the number of samples (MSXAA) that multisampled renderers should
        // use
        int samples() const noexcept;

        // returns true if the application is rendering in debug mode (i.e. whether
        // downstream rendererers should also render debug info)
        bool is_in_debug_mode() const noexcept;
    };
}
