#pragma once

#include "sdl.hpp"
#include "imgui_extensions.hpp"

#include <memory>

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, imgui, SDL), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a `Screen`

namespace osmv {
    struct Screen;

    struct Application final {
        sdl::Context context;
        sdl::Window window;
        sdl::GLContext gl;
        igx::Context imgui_ctx;
        igx::SDL2_Context imgui_sdl2_ctx;
        igx::OpenGL3_Context imgui_sdl2_ogl2_ctx;

        // true if the framerate should be throttled to a reasonable-ish rate
        // (e.g. ~60 FPS) by `sleep`ing
        bool software_throttle = true;

        // must be bootstrapped after init-ing a screen
        std::unique_ptr<Screen> current_screen = nullptr;

        Application();
        ~Application() noexcept;

        void show();
    };
}
