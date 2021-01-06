#pragma once

#include "sdl.hpp"
#include "imgui_extensions.hpp"

#include <memory>

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance

namespace osmv {
    struct Screen;

    class Application final {
        sdl::Context context;
        sdl::Window window;
        sdl::GLContext gl;
        igx::Context imgui_ctx;
        igx::SDL2_Context imgui_sdl2_ctx;
        igx::OpenGL3_Context imgui_sdl2_ogl2_ctx;
        bool software_throttle = true;
        std::unique_ptr<Screen> current_screen;

    public:
        Application();
        Application(Application const&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application const&) = delete;
        Application& operator=(Application&&) = delete;
        ~Application() noexcept = default;

        // start showing application window with an initial screen
        void show(std::unique_ptr<osmv::Screen>);

        // true if FPS is being throttled (e.g. with software (sleeps) or vsync)
        bool is_fps_throttling() const noexcept {
            return software_throttle;
        }

        void enable_fps_throttling() noexcept {
            software_throttle = true;
        }

        void disable_fps_throttling() noexcept {
            software_throttle = false;
        }

        // dimensions of the main application window in pixels
        sdl::Window_dimensions window_size() const noexcept {
            return sdl::GetWindowSize(window);
        }

        float aspect_ratio() const noexcept {
            auto [w, h] = window_size();
            return static_cast<float>(w) / static_cast<float>(h);
        }

        // move mouse relative to the window (origin in top-left)
        void move_mouse_to(int x, int y) {
            SDL_WarpMouseInWindow(window, x, y);
        }
    };
}
