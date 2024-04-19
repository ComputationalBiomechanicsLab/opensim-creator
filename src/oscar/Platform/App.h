#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppClock.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Platform/Screenshot.h>

#include <SDL_events.h>

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

struct SDL_Window;
namespace osc { struct Color; }
namespace osc { class AppConfig; }
namespace osc { class AppMetadata; }
namespace osc { class IScreen; }
namespace osc::ui::context { void init(); }

namespace osc
{
    // top-level application class
    //
    // the top-level osc process holds one copy of this class, which maintains all global
    // systems (windowing, event pumping, timers, graphics, logging, etc.)
    class App {
    public:
        template<std::destructible T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        static std::shared_ptr<T> singleton(Args&&... args)
        {
            const auto ctor = [args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> std::shared_ptr<void>
            {
                return std::apply([](auto&&... inner_args) -> std::shared_ptr<void>
                {
                     return std::make_shared<T>(std::forward<Args>(inner_args)...);
                }, std::move(args_tuple));
            };

            return std::static_pointer_cast<T>(App::upd().upd_singleton(typeid(T), ctor));
        }

        // returns the currently-active application global
        static App& upd();
        static const App& get();

        static const AppConfig& config();

        // returns a full filesystem path to a (runtime- and configuration-dependent) application resource
        static std::filesystem::path resource_filepath(const ResourcePath&);

        // returns the contents of a runtime resource in the `resources/` dir as a string
        static std::string slurp(const ResourcePath&);

        // returns an opened stream to the given application resource
        static ResourceStream load_resource(const ResourcePath&);

        // returns the top- (application-)level resource loader
        static ResourceLoader& resource_loader();

        // constructs an `App` from a default-constructed `AppMetadata`
        App();

        // constructs an app by initializing it from a config at the default app config location
        //
        // this also sets the `cur` application global
        explicit App(const AppMetadata&);
        App(const App&) = delete;
        App(App&&) noexcept;
        App& operator=(const App&) = delete;
        App& operator=(App&&) noexcept;
        ~App() noexcept;

        // returns the application's metadata (name, organization, repo URL, version, etc.)
        const AppMetadata& metadata() const;

        // returns the filesystem path to the current application executable
        const std::filesystem::path& executable_dir() const;

        // returns the filesystem path to a (usually, writable) user-specific directory for the
        // application
        const std::filesystem::path& user_data_dir() const;

        // start showing the supplied screen, only returns once a screen requests to quit or an exception is thrown
        void show(std::unique_ptr<IScreen>);

        // construct `TScreen` with `Args` and start showing it
        template<std::derived_from<IScreen> TScreen, typename... Args>
        requires std::constructible_from<TScreen, Args&&...>
        void show(Args&&... args)
        {
            show(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // Request the app transitions to a new screen
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level application
        // loop).
        //
        // When the App decides it's ready to transition to the new screen, it will:
        //
        // - unmount the current screen
        // - destroy the current screen
        // - mount the new screen
        // - make the new screen the current screen
        void request_transition(std::unique_ptr<IScreen>);

        // construct `TScreen` with `Args` then request the app transitions to it
        template<std::derived_from<IScreen> TScreen, typename... Args>
        requires std::constructible_from<TScreen, Args&&...>
        void request_transition(Args&&... args)
        {
            request_transition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request that the app quits
        //
        // this is merely a *request* tha the `App` will fulfill at a later time (usually,
        // after it's done handling some part of the top-level application loop)
        void request_quit();

        // returns main window's dimensions (float)
        Vec2 dimensions() const;

        // sets whether the user's mouse cursor should be shown/hidden
        void set_show_cursor(bool);

        // makes the main window fullscreen
        void make_fullscreen();

        // makes the main window fullscreen, but still composited with the desktop (so-called 'windowed maximized' in games)
        void make_windowed_fullscreen();

        // makes the main window windowed (as opposed to fullscreen)
        void make_windowed();

        // returns the recommended number of MSXAA antiAliasingLevel that renderers should use (based on config etc.)
        AntiAliasingLevel anti_aliasing_level() const;

        // sets the number of MSXAA antiAliasingLevel multisampled renderers should use
        //
        // throws if arg > max_samples()
        void set_anti_aliasing_level(AntiAliasingLevel);

        // returns the maximum number of MSXAA antiAliasingLevel the backend supports
        AntiAliasingLevel max_anti_aliasing_level() const;

        // returns true if the application is rendering in debug mode
        //
        // other parts of the application can use this to decide whether to render
        // extra debug elements, etc.
        bool is_in_debug_mode() const;
        void enable_debug_mode();
        void disable_debug_mode();

        // returns true if VSYNC has been enabled in the graphics layer
        bool is_vsync_enabled() const;
        void set_vsync(bool);
        void enable_vsync();
        void disable_vsync();

        // add an annotation to the current frame
        //
        // the annotation is added to the data returned by `App::request_screenshot`
        void add_frame_annotation(std::string_view label, Rect screen_rect);

        // returns a future that asynchronously yields a complete annotated screenshot of the next frame
        //
        // client code can submit annotations with `App::add_frame_annotation`
        std::future<Screenshot> request_screenshot();

        // returns human-readable strings representing (parts of) the graphics backend (e.g. OpenGL)
        std::string graphics_backend_vendor_string() const;
        std::string graphics_backend_renderer_string() const;
        std::string graphics_backend_version_string() const;
        std::string graphics_backend_shading_language_version_string() const;

        // returns the number of times the application has drawn a frame to the screen
        size_t num_frames_drawn() const;

        // returns the time at which the app started up (arbitrary timepoint, don't assume 0)
        AppClock::time_point startup_time() const;

        // returns the time delta between when the app started up and the current frame
        AppClock::duration frame_delta_since_startup() const;

        // returns the time at which the current frame started
        AppClock::time_point frame_start_time() const;

        // returns the time delta between when the current frame started and when the previous
        // frame started
        AppClock::duration frame_delta_since_last_frame() const;

        // makes main application event loop wait, rather than poll, for events
        //
        // By default, `App` is a *polling* event loop that renders as often as possible. This
        // method makes the main application a *waiting* event loop that only moves forward when
        // an event occurs.
        //
        // Rendering this way is *much* more power efficient (especially handy on TDP-limited devices
        // like laptops), but downstream screens *must* ensure the application keeps moving forward by
        // calling methods like `request_redraw` or by pumping other events into the loop.
        bool is_main_loop_waiting() const;
        void set_main_loop_waiting(bool);
        void make_main_loop_waiting();
        void make_main_loop_polling();
        void request_redraw();  // threadsafe: used to make a waiting loop redraw

        // fill all pixels in the screen with the given color
        void clear_screen(const Color&);

        // sets the main window's subtitle (e.g. document name)
        void set_main_window_subtitle(std::string_view);

        // unsets the main window's subtitle
        void unset_main_window_subtitle();

        // returns the current application configuration
        const AppConfig& get_config() const;
        AppConfig& upd_config();

        // returns the top- (application-)level resource loader
        ResourceLoader& upd_resource_loader();

        // returns the contents of a runtime resource in the `resources/` dir as a string
        std::string slurp_resource(const ResourcePath&);

        // returns an opened stream to the given resource
        ResourceStream go_load_resource(const ResourcePath&);

    private:
        // returns a full filesystem path to runtime resource in `resources/` dir
        std::filesystem::path get_resource_filepath(const ResourcePath&) const;

        // try and retrieve a virtual singleton that has the same lifetime as the app
        std::shared_ptr<void> upd_singleton(const std::type_info&, const std::function<std::shared_ptr<void>()>&);

        // HACK: the 2D ui currently needs access to these
        SDL_Window* upd_underlying_window();
        void* upd_underlying_opengl_context();
        friend void ui::context::init();

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
