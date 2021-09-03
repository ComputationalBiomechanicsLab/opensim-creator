#pragma once

#include "src/assertions.hpp"
#include "src/recent_file.hpp"
#include "src/screen.hpp"

#include <SDL_events.h>
#include <glm/vec2.hpp>

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace osc {
    struct Config;
}

namespace osc {

    class App final {
        // set when App is constructed for the first time
        static App* g_Current;

    public:
        struct Impl;
        std::unique_ptr<Impl> impl;

        [[nodiscard]] static App& cur() noexcept {
            OSC_ASSERT(g_Current && "App is not initialized: have you constructed a (singleton) instance of App?");
            return *g_Current;
        }

        [[nodiscard]] static Config const& config() noexcept {
            return cur().get_config();
        }

        [[nodiscard]] static std::filesystem::path resource(std::string_view s) {
            return cur().get_resource(s);
        }

        // init app by loading config from default location
        App();
        App(App const&) = delete;
        App(App&&) noexcept;
        ~App() noexcept;

        App& operator=(App const&) = delete;
        App& operator=(App&&) noexcept;

        // start showing the supplied screen
        void show(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` and start showing it
        template<typename TScreen, typename... Args>
        void show(Args&&... args) {
            show(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request the app transitions to a new sreen
        //
        // this is a *request* that `App` will fulfill at a later time. App will
        // first call `on_unmount` on the current screen, fully destroy the current
        // screen, then call `on_mount` on the new screen and make the new screen
        // the current screen
        void request_transition(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` then request the app transitions to it
        template<typename TScreen, typename... Args>
        void request_transition(Args&&... args) {
            request_transition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request the app quits as soon as it can (usually after it's finished with a
        // screen method)
        void request_quit();

        // returns current window dimensions (integer)
        [[nodiscard]] glm::ivec2 idims() const noexcept;

        // returns current window dimensions (float)
        [[nodiscard]] glm::vec2 dims() const noexcept;

        // returns current window aspect ratio
        [[nodiscard]] float aspect_ratio() const noexcept;

        // hides mouse in screen and makes it operate relative per-frame
        void set_relative_mouse_mode() noexcept;

        // makes the application window fullscreen
        void make_fullscreen();

        // makes the application window windowed (as opposed to fullscreen)
        void make_windowed();

        // returns number of MSXAA samples multisampled rendererers should use
        [[nodiscard]] int get_samples() const noexcept;

        // sets the number of MSXAA samples multisampled renderered should use
        //
        // throws if arg > max_samples()
        void set_samples(int);

        // returns the maximum number of MSXAA samples the backend supports
        [[nodiscard]] int max_samples() const noexcept;

        // returns true if the application is rendering in debug mode
        //
        // screen/tab/widget implementations should use this to decide whether
        // to draw extra debug elements
        [[nodiscard]] bool is_in_debug_mode() const noexcept;
        void enable_debug_mode();
        void disable_debug_mode();

        [[nodiscard]] bool is_vsync_enabled() const noexcept;
        void enable_vsync();
        void disable_vsync();

        [[nodiscard]] Config const& get_config() const noexcept;

        // get full path to runtime resource in `resources/` dir
        [[nodiscard]] std::filesystem::path get_resource(std::string_view) const noexcept;

        // returns the contents of a resource in a string
        [[nodiscard]] std::string slurp_resource(std::string_view) const;

        // returns all files that were recently opened by the user in the app
        //
        // the list is persisted between app boots
        [[nodiscard]] std::vector<Recent_file> recent_files() const;

        // add a file to the recently opened files list
        //
        // this addition is persisted between app boots
        void add_recent_file(std::filesystem::path const&);
    };

    // ImGui support
    //
    // these methods are specialized for OSC (config, fonts, themeing, etc.)
    //
    // these methods should be called by each `Screen` implementation. The reason they aren't
    // automatically integrated into App/Screen is because some screens might want very tight
    // control over ImGui (e.g. recycling contexts, aggro-resetting contexts)

    void ImGuiInit();  // init ImGui context (/w osc settings)
    void ImGuiShutdown();  // shutdown ImGui context
    bool ImGuiOnEvent(SDL_Event const&);  // returns true if ImGui has handled the event
    void ImGuiNewFrame();  // should be called at the start of `draw()`
    void ImGuiRender();  // should be called at the end of `draw()`
}
