#pragma once

#include <cassert>
#include <memory>
#include <utility>

namespace osmv {
    class Screen;
    class Rendering_system;
    class Model_decoration_generator;
}
namespace osmv {
    struct Application_impl;
}

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance
namespace osmv {
    // custom events that OSMV may push into the SDL event queue
    enum OsmvCustomEvent {
        // number of MSXAA samples changed - implementations should check if they need to
        // reallocate any render buffers
        OsmvCustomEvent_SamplesChanged,
    };

    struct Window_dimensions final {
        int w;
        int h;
    };

    class Application final {
        static Application* gCurrent;

        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        static void set_current(Application* ptr) {
            gCurrent = ptr;
        }

        static Application& current() noexcept {
            assert(gCurrent != nullptr);
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

        // dimensions of the main application window in pixels
        Window_dimensions window_dimensions() const noexcept;

        float window_aspect_ratio() const noexcept {
            auto [w, h] = window_dimensions();
            return static_cast<float>(w) / static_cast<float>(h);
        }

        // move mouse relative to the window (origin in top-left)
        void move_mouse_to(int x, int y);

        // returns the number of samples (MSXAA) that multisampled renderers should use
        int samples() const noexcept;

        // returns the maximum number of samples (MSXAA) that the OpenGL backend supports
        int max_samples() const noexcept;

        // set the number of samples (MSXAA) that multisampled renderers should use
        void set_samples(int);

        // returns true if the application is rendering in debug mode (i.e. whether
        // downstream rendererers should also render debug info)
        bool is_in_debug_mode() const noexcept;

        // makes the application window fullscreen
        void make_fullscreen();

        // makes the application window windowed (as opposed to fullscreen)
        void make_windowed();

        bool is_vsync_enabled() const noexcept;

        void enable_vsync();

        void disable_vsync();
    };
}
