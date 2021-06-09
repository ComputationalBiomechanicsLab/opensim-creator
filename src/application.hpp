#pragma once

#include "src/assertions.hpp"

#include "third_party/IconsFontAwesome5.h"

#include <memory>
#include <utility>

namespace osc {
    class Screen;
    struct GPU_storage;
}

// application: top-level application state
//
// this top-level class is responsible for initializing a bare-minimum subset
// of the UI's functionality (e.g. OpenGL, SDL, ImGui), and handling
// application-level upkeep (event pumping, throttling, etc.) while deferring
// actual per-screen rendering work to a (changing) `Screen` instance
namespace osc {


    class Application final {
        static Application* g_Current;

        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        static void set_current(Application* ptr) noexcept {
            g_Current = ptr;
        }

        [[nodiscard]] static Application& current() noexcept {
            OSC_ASSERT(g_Current != nullptr);
            return *g_Current;
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

        void request_screen_transition(std::unique_ptr<osc::Screen>);

        template<typename Screen, typename... Args>
        void request_screen_transition(Args&&... args) {
            request_screen_transition(std::make_unique<Screen>(std::forward<Args>(args)...));
        }

        void request_quit_application();

        struct Window_dimensionsi final { int w, h; };
        [[nodiscard]] Window_dimensionsi window_dimensionsi() const noexcept;

        struct Window_dimensionsf final { float w, h; };
        [[nodiscard]] Window_dimensionsf window_dimensionsf() const noexcept;

        // returns the number of samples (MSXAA) that multisampled renderers should use
        [[nodiscard]] int samples() const noexcept;

        // returns the maximum number of samples (MSXAA) that the OpenGL backend supports
        [[nodiscard]] int max_samples() const noexcept;

        // set the number of samples (MSXAA) that multisampled renderers should use
        void set_samples(int);

        // returns true if the application is rendering in debug mode
        [[nodiscard]] bool is_in_debug_mode() const noexcept;
        void enable_debug_mode();
        void disable_debug_mode();

        // makes the application window fullscreen
        void make_fullscreen();

        // makes the application window windowed (as opposed to fullscreen)
        void make_windowed();

        [[nodiscard]] bool is_vsync_enabled() const noexcept;
        void enable_vsync();
        void disable_vsync();

        GPU_storage& get_gpu_storage() noexcept;

        // try to forcibly reset ImGui's state
        //
        // this is necessary when (e.g.) an exception throws all the way through
        // ImGui `End` methods (e.g. `EndChild`) and has potentially clobbered ImGui
        // (which is not exception-safe)
        void reset_imgui_state();
    };
}
