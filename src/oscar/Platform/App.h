#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppClock.h>
#include <oscar/Platform/AppMainLoopStatus.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Platform/Monitor.h>
#include <oscar/Platform/Screenshot.h>
#include <oscar/Platform/WindowID.h>

#include <concepts>
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

namespace osc { class AppSettings; }
namespace osc { class AppMetadata; }
namespace osc { class Cursor; }
namespace osc { class Event; }
namespace osc { class Screen; }
namespace osc { class Widget; }
namespace osc::ui::context { void init(App&); }

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
            const auto singleton_constructor = [args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> std::shared_ptr<void>
            {
                return std::apply([](auto&&... inner_args) -> std::shared_ptr<void>
                {
                     return std::make_shared<T>(std::forward<Args>(inner_args)...);
                }, std::move(args_tuple));
            };

            return std::static_pointer_cast<T>(App::upd().upd_singleton(typeid(T), singleton_constructor));
        }

        // returns the currently-active application global
        static App& upd();
        static const App& get();

        static const AppSettings& settings();

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

        // constructs an app by initializing it from a settings at the default app settings location
        //
        // this also sets the currently-active application global (i.e. `App::upd()` and `App::get()` will work)
        explicit App(const AppMetadata&);
        App(const App&) = delete;
        App(App&&) noexcept = delete;
        App& operator=(const App&) = delete;
        App& operator=(App&&) noexcept = delete;
        ~App() noexcept;

        // returns the application's metadata (name, organization, repo URL, version, etc.)
        const AppMetadata& metadata() const;

        // returns the filesystem path to the current application executable
        const std::filesystem::path& executable_directory() const;

        // returns the filesystem path to a (usually, writable) user-specific directory for the
        // application
        const std::filesystem::path& user_data_directory() const;

        void setup_main_loop(std::unique_ptr<Screen>);
        template<std::derived_from<Screen> TScreen, typename... Args>
        requires std::constructible_from<TScreen, Args&&...>
        void setup_main_loop(Args&&... args)
        {
            setup_main_loop(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }
        AppMainLoopStatus do_main_loop_step();
        void teardown_main_loop();

        // Adds `event`, with the widget `receiver` as the receiver of `event`, to the event
        // queue and returns immediately.
        //
        // When the event is popped off the event queue, it is processed as-if by calling
        // `notify(receiver, *event)`. See the documentation for `notify` for a detailed
        // description of event processing.
        static void post_event(Widget& receiver, std::unique_ptr<Event> event);
        template<std::derived_from<Event> TEvent, typename... Args>
        requires std::constructible_from<TEvent, Args&&...>
        static void post_event(Widget& receiver, Args&&... args)
        {
            return post_event(receiver, std::make_unique<TEvent>(std::forward<Args>(args)...));
        }

        // Immediately sends `event` to `receiver` as-if by calling `return receiver.on_event(event)`.
        //
        // This application-level event handler behaves differently from directly calling
        // `receiver.on_event(event)` because it also handles event propagation. The implementation
        // will call `Widget::on_event(Event&)` for each `Widget` from `receiver` to the root widget
        // until either a widget in that chain returns `true` or `event.propagates()` is `false`.
        static bool notify(Widget& receiver, Event& event);
        template<std::derived_from<Event> TEvent, typename... Args>
        requires std::constructible_from<TEvent, Args&&...>
        static bool notify(Widget& receiver, Args&&... args)
        {
            TEvent event{std::forward<Args>(args)...};
            return notify(receiver, event);
        }

        // sets the currently active screen, creates an application loop, then starts showing
        // the supplied screen
        //
        // this function only returns once the active screen calls `app.request_quit()`, or an exception
        // is thrown. Use `set_screen` in combination with `handle_one_frame` if you want to use your
        // own application loop
        //
        // this is effectively sugar over:
        //
        //     set_screen(...);
        //     setup_main_loop();
        //     while (true) {
        //         do_main_loop_step(...);
        //     }
        //     teardown_main_loop();
        //
        // which you may need to write yourself if your loop is external (e.g. from a browser's event loop)
        void show(std::unique_ptr<Screen>);

        // constructs `TScreen` with `Args` and starts `show`ing it
        template<std::derived_from<Screen> TScreen, typename... Args>
        requires std::constructible_from<TScreen, Args&&...>
        void show(Args&&... args)
        {
            show(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // requests that the app transitions to a new screen
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level
        // application rendering loop)
        //
        // When the App decides it's ready to transition to the new screen, it will:
        //
        // - unmount the current screen
        // - destroy the current screen
        // - mount the new screen
        // - make the new screen the current screen
        void request_transition(std::unique_ptr<Screen>);

        // constructs `TScreen` with `Args` then requests that the app transitions to it
        template<std::derived_from<Screen> TScreen, typename... Args>
        requires std::constructible_from<TScreen, Args&&...>
        void request_transition(Args&&... args)
        {
            request_transition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // requests that the app quits
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level
        // application rendering loop)
        void request_quit();

        // returns a sequence of all physical monitors associated with the windowing system that
        // this `App` is connected to.
        std::vector<Monitor> monitors() const;

        // checks if the given `WindowID` is still alive (e.g. not removed or closed)
        bool is_alive(WindowID window_id) const { return static_cast<bool>(window_id); }

        // Returns the desktop-relative position of the given window in physical pixels.
        Vec2 window_position(WindowID) const;

        // returns the ID of the main window
        WindowID main_window_id() const;

        // Returns the dimensions of the main application window in device-independent pixels.
        Vec2 main_window_dimensions() const;

        // Returns the dimensions of the main application window in physical pixels.
        Vec2 main_window_pixel_dimensions() const;

        // Returns the ratio of the resolution in physical pixels to the resolution of
        // device-independent pixels.
        //
        // E.g. a high DPI monitor might return `2.0f`, which means "two physical pixels
        // along X and Y map to one device-independent pixel".
        //
        // Related (other libraries):
        //
        // - https://developer.mozilla.org/en-US/docs/Web/API/Window/devicePixelRatio
        // - https://doc.qt.io/qt-6/highdpi.html
        // - https://doc.qt.io/qt-6/qwindow.html#devicePixelRatio
        // - https://github.com/libsdl-org/SDL/blob/main/docs/README-highdpi.md
        float main_window_device_pixel_ratio() const;

        // Returns the ratio between the underlying operating system's coordinate system (i.e. SDL3 API,
        // events) to the main window's device independent pixels.
        //
        // Note: this is mostly for internal use: the `osc` APIs should uniformly use either device
        //       independent pixels (mostly) or physical pixels (e.g. low-level rendering APIs).
        float os_to_main_window_device_independent_ratio() const;

        // Returns the ratio between the main window's device-independent pixels and the underlying
        // operating system's coordinate system.
        float main_window_device_independent_to_os_ratio() const;

        // returns `true` if the main application window is minimized
        bool is_main_window_minimized() const;

        // returns `true` if mouse data can be acquired from the operating system directly
        bool can_query_mouse_state_globally() const;

        // captures the mouse in order to track input outside of the application windows
        //
        // capturing enables the application to obtain mouse events globally, rather than just
        // within its windows. Not all backends support this function, use `can_query_mouse_state_globally`
        // to figure out whether the backend that's used at runtime does support it.
        //
        // this might also deny mouse inputs to other windows--both those in this `App`, and others
        // on the system--so it should be used sparingly, and in small bursts. E.g. you might want
        // to use it to track the mouse when the user is dragging something and multi-window dragging
        // is supported by the UI.
        //
        // see: https://wiki.libsdl.org/SDL3/SDL_CaptureMouse for a comprehensive explanation of the
        //      behavior/pitfalls of this function.
        void capture_mouse_globally(bool enabled);

        // returns the desktop-relative platform-cursor position, expressed in physical pixels.
        Vec2 mouse_global_position() const;

        // moves the mouse to the given position in a desktop-relative, physical pixel position.
        void warp_mouse_globally(Vec2 new_position);

        // returns `true` if the global hover state of the mouse can be queried to ask if it's
        // currently hovering the main window (even if the window isn't focused).
        bool can_query_if_mouse_is_hovering_main_window_globally() const;

        // pushes the given cursor onto the application-wide cursor stack, making it
        // the currently-active cursor until it is either popped, via `pop_cursor_override`,
        // or another cursor is pushed.
        void push_cursor_override(const Cursor&);
        void pop_cursor_override();

        // enables/disables "grabbing" the mouse cursor in the main window
        void enable_main_window_grab();
        void disable_main_window_grab();

        // moves the mouse cursor to the given position within the window (virtual pixels).
        void warp_mouse_in_window(WindowID, Vec2);

        // returns `true` if the given window has input focus
        bool has_input_focus(WindowID) const;

        // returns the ID of the window, if any, that currently has the user's keyboard focus.
        //
        // a default-constructed `WindowID` is returned if no window has keyboard focus.
        WindowID get_keyboard_focus() const;

        // sets the rectangle that's used to type unicode text inputs
        //
        // native input methods can place a window with word suggestions near the input
        // in the UI, without covering the text that's being inputted, this indicates to
        // the OS where the input rectangle is so that it can place the overlay in the
        // correct location.
        void set_unicode_input_rect(const Rect&);

        // start accepting unicode text input events for the given window
        //
        // it's usually necessary to call `set_unicode_input_rect` before calling this, so that
        // the text input UI is placed correctly.
        void start_text_input(WindowID);

        // stop accepting unicode text input events for the given window
        void stop_text_input(WindowID);

        // makes the main window fullscreen, but still composited with the desktop (so-called 'windowed maximized' in games)
        void make_windowed_fullscreen();

        // makes the main window windowed (as opposed to fullscreen)
        void make_windowed();

        // returns the recommended number of antialiasing samples that renderers that want to render
        // to this `App`'s screen should use (based on user settings, etc.)
        AntiAliasingLevel anti_aliasing_level() const;

        // sets the number of antialiasing samples that multi-sampled renderers should use when they
        // want to render to this `App`'s screen
        //
        // throws if `samples > max_samples()`
        void set_anti_aliasing_level(AntiAliasingLevel);

        // returns the maximum number of antialiasing samples that the graphics backend supports
        AntiAliasingLevel max_anti_aliasing_level() const;

        // returns true if the main window is backed by a framebuffer/renderbuffer that automatically
        // converts the linear outputs (from shaders) into (e.g.) sRGB on-write
        bool is_main_window_gamma_corrected() const;

        // returns true if the application is rendering in debug mode
        //
        // other parts of the application can use this to decide whether to render
        // extra debug elements, etc.
        bool is_in_debug_mode() const;
        void set_debug_mode(bool);

        // returns true if VSYNC has been enabled in the graphics layer
        bool is_vsync_enabled() const;
        void set_vsync_enabled(bool);

        // add an annotation to the current frame
        //
        // the annotation is added to the data returned by `App::request_screenshot`
        void add_frame_annotation(std::string_view label, Rect screen_rect);

        // returns a future that asynchronously yields a complete annotated screenshot of the next frame
        //
        // client code can submit annotations with `App::add_frame_annotation`
        std::future<Screenshot> request_screenshot();

        // returns human-readable strings representing (parts of) the currently-active graphics backend (e.g. OpenGL)
        std::string graphics_backend_vendor_string() const;
        std::string graphics_backend_renderer_string() const;
        std::string graphics_backend_version_string() const;
        std::string graphics_backend_shading_language_version_string() const;

        // returns the number of times this `App` has drawn a frame to the screen
        size_t num_frames_drawn() const;

        // returns the time at which this `App` started up (arbitrary timepoint, don't assume 0)
        AppClock::time_point startup_time() const;

        // returns `frame_start_time() - startup_time()`
        AppClock::duration frame_delta_since_startup() const;

        // returns the time at which the current frame started being drawn
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

        // fill all pixels in the main window with the given color
        void clear_screen(const Color& = Color::clear());

        // sets the main window's subtitle (e.g. document name)
        void set_main_window_subtitle(std::string_view);

        // unsets the main window's subtitle
        void unset_main_window_subtitle();

        // returns the current application configuration
        const AppSettings& get_config() const;
        AppSettings& upd_settings();

        // returns the top- (application-)level resource loader
        ResourceLoader& upd_resource_loader();

        // returns the contents of a runtime resource in the `resources/` dir as a string
        std::string slurp_resource(const ResourcePath&);

        // returns an opened stream to the given resource
        ResourceStream go_load_resource(const ResourcePath&);

    private:
        // returns a full filesystem path to runtime resource in `resources/` dir
        std::filesystem::path get_resource_filepath(const ResourcePath&) const;

        // try and retrieve a singleton that has the same lifetime as the app
        std::shared_ptr<void> upd_singleton(const std::type_info&, const std::function<std::shared_ptr<void>()>&);

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
