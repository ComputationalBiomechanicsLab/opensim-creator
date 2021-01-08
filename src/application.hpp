#pragma once

#include "sdl.hpp"
#include <memory>

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance
namespace osmv {
    struct Screen;
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

        void show(std::unique_ptr<osmv::Screen>);

        // experimental feature: the UI's event loop waits for events, rather than polling
        //
        // this is currently very buggy, because parts of the UI depend on polling
        bool waiting_event_loop() const noexcept;
        void waiting_event_loop(bool);
        void request_redraw();

        // true if FPS is being throttled (e.g. with software (sleeps) or vsync)
        bool fps_throttling() const noexcept;
        void fps_throttling(bool);

        // dimensions of the main application window in pixels
        sdl::Window_dimensions window_size() const noexcept;

        float aspect_ratio() const noexcept;

        // move mouse relative to the window (origin in top-left)
        void move_mouse_to(int x, int y);
    };
}
