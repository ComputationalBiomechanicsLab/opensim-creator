#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/AppClock.h>
#include <liboscar/Platform/AppMainLoopStatus.h>
#include <liboscar/Platform/FileDialogFilter.h>
#include <liboscar/Platform/FileDialogResponse.h>
#include <liboscar/Platform/ResourceLoader.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Platform/Screenshot.h>
#include <liboscar/Platform/WindowID.h>

#include <concepts>
#include <filesystem>
#include <functional>
#include <future>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>

namespace osc { class AppMetadata; }
namespace osc { class AppSettings; }
namespace osc { class Cursor; }
namespace osc { class Event; }
namespace osc { class FileDialogResponse; }
namespace osc { class Widget; }

namespace osc
{
    // top-level application class
    //
    // the top-level osc process holds one copy of this class, which maintains all
    // application-wide systems (windowing, event pumping, timers, graphics,
    // logging, etc.)
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

        // Convenience function that initializes an instance of `App` according to the target
        // platform's requirements and immediately starts showing the given `TWidget` according
        // to the target platform's main application loop requirements.
        //
        // This function should only be called once per-process, and should be the last statement
        // in the application's `main` function (i.e. `return App::main(...)` from `main`), because
        // the target platform might have unusual lifetime behavior (e.g. web browsers may continue
        // to run after `main` has completed).
        template<std::derived_from<Widget> TWidget, typename... Args>
        requires std::constructible_from<TWidget, Args&&...>
        static int main(const AppMetadata& metadata, Args&&... args)
        {
            // Pack the `TWidget` constructor arguments into a type-erased `std::function` and defer
            // to the internal implementation.
            return main_internal(metadata, [args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> std::unique_ptr<Widget>
            {
                return std::apply([](auto&&... inner_args) -> std::unique_ptr<Widget>
                {
                    return std::make_unique<TWidget>(std::forward<Args>(inner_args)...);
                }, std::move(args_tuple));
            });
        }

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

        // returns a human-readable (i.e. may be long-form) representation of the application name
        std::string human_readable_name() const;

        // returns a string representation of the name of the application, its version, and
        // its build id (usually useful for logging, file headers, etc.)
        std::string application_name_with_version_and_buildid() const;

        // returns the filesystem path to the current application executable
        const std::filesystem::path& executable_directory() const;

        // returns the filesystem path to a (usually, writable) user-specific directory for the
        // application
        const std::filesystem::path& user_data_directory() const;

        void setup_main_loop(std::unique_ptr<Widget>);
        template<std::derived_from<Widget> TWidget, typename... Args>
        requires std::constructible_from<TWidget, Args&&...>
        void setup_main_loop(Args&&... args)
        {
            setup_main_loop(std::make_unique<TWidget>(std::forward<Args>(args)...));
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

        // sets the currently active widget, creates an application loop, then starts showing
        // the supplied widget
        //
        // this function only returns once the active widget calls `app.request_quit()`, or an exception
        // is thrown. Use `set_widget` in combination with `handle_one_frame` if you want to use your
        // own application loop
        //
        // this is effectively sugar over:
        //
        //     set_widget(...);
        //     setup_main_loop();
        //     while (true) {
        //         do_main_loop_step(...);
        //     }
        //     teardown_main_loop();
        //
        // which you may need to write yourself if your loop is external (e.g. from a browser's event loop)
        void show(std::unique_ptr<Widget>);

        // constructs `TWidget` with `Args` and starts `show`ing it
        template<std::derived_from<Widget> TWidget, typename... Args>
        requires std::constructible_from<TWidget, Args&&...>
        void show(Args&&... args)
        {
            show(std::make_unique<TWidget>(std::forward<Args>(args)...));
        }

        // requests that the application's main window transitions to a new
        // top-level widget
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level
        // application rendering loop)
        //
        // When the App decides it's ready to transition to the new widget, it will:
        //
        // - unmount the current widget
        // - destroy the current widget
        // - mount the new widget
        // - make the new widget the current top-level widget
        void request_transition(std::unique_ptr<Widget>);

        // constructs `TWidget` with `Args` then requests that the app transitions to it
        template<std::derived_from<Widget> TWidget, typename... Args>
        requires std::constructible_from<TWidget, Args&&...>
        void request_transition(Args&&... args)
        {
            request_transition(std::make_unique<TWidget>(std::forward<Args>(args)...));
        }

        // requests that the app quits
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level
        // application rendering loop)
        void request_quit();

        // Requests that the given function is executed on the main thread.
        //
        // Main thread means "the thread that's responsible for pumping the main event queue".
        // Usually, this is whichever thread is calling `show` or `do_main_loop_step`. The
        // callback may NOT be called if the application quits, or is destructed before being
        // able to process all events.
        void request_invoke_on_main_thread(std::function<void()>);

        // Gets/sets the directory that should be shown to the user if a call to one of the
        // `prompt_user*` files does not provide an `initial_directory_to_show`. If this
        // fallback isn't provided, the implementation will fall back to whatever the
        // OS's default behavior is (typically, it remembers the user's last usage).
        //
        // This fallback is activated until a call to `prompt_user*` is made without the
        // user cancelling out of the dialog (i.e. if the user cancels then this fallback
        // will remain in-place).
        std::optional<std::filesystem::path> prompt_initial_directory_to_show_fallback();
        void set_prompt_initial_directory_to_show_fallback(const std::filesystem::path&);
        void set_prompt_initial_directory_to_show_fallback(std::nullopt_t);  // reset it

        // Prompts the user to select file(s) that they would like to open.
        //
        // - `callback` is called from the ui thread by the implementation when the user chooses
        //   a file, cancels, or there's an error. It is implementation-defined whether `callback`
        //   is called immediately or as part of pumping the application event loop. `callback`
        //   may not be called if the application quits/destructs prematurely.
        //
        // - `filters` should be a sequence of permitted `FileDialogFilter`s, which will constrain
        //   which files the user can select in the dialog in an implementation-defined way.
        //
        // - `initial_directory_to_show` should be a filesystem path to a directory that should
        //   initially be shown to the user. If it isn't provided, then an implementation-defined
        //   directory will be shown (e.g. based on previous user choices, OS defaults, etc.).
        void prompt_user_to_select_file_async(
            std::function<void(FileDialogResponse&&)> callback,
            std::span<const FileDialogFilter> filters = {},
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt,
            bool allow_many = false
        );
        void prompt_user_to_select_file_async(
            std::function<void(FileDialogResponse&&)> callback,
            std::initializer_list<const FileDialogFilter> filters = {},
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt,
            bool allow_many = false)
        {
            return prompt_user_to_select_file_async(
                std::move(callback),
                std::span<const FileDialogFilter>{filters},
                std::move(initial_directory_to_show),
                allow_many
            );
        }

        // Prompts the user to select a single existing directory.
        //
        // - `callback` is called from the ui thread by the implementation when the user chooses
        //   a file, cancels, or there's an error. It is implementation-defined whether `callback`
        //   is called immediately or as part of pumping the application event loop. `callback` may
        //   not be called if the application quits/destructs prematurely.
        //
        // - `initial_directory_to_show` should be a filesystem path to a directory that should
        //   initially be shown to the user. If it isn't provided, then an implementation-defined
        //   directory will be shown (e.g. based on previous user choices, OS defaults, etc.).
        //
        // - `allow_many` indicates whether the user can select multiple directories. However, not
        //   all implementations support this option.
        void prompt_user_to_select_directory_async(
            std::function<void(FileDialogResponse&&)> callback,
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt,
            bool allow_many = false
        );

        // Prompts the user to select a new or existing filesystem path where they would like
        // to save a file.
        //
        // - `callback` is called from the main thread by the implementation when the user chooses
        //   a file, cancels or there's an error. It is implementation-defined whether `callback` is
        //   called  immediately, or as part of pumping the application event loop. `callback` may
        //   not be called if the application quits/destructs prematurely.
        //
        // - `filters` should be a sequence of permitted `FileDialogFilter`s, which will constrain
        //   which file extensions the user can use in the dialog in an implementation-defined way.
        //
        // - `initial_directory_to_show` should be a filesystem path to a directory that should
        //   initially be shown to the user. If it isn't provided, then an implementation-defined
        //   directory will be shown (e.g. based on previous user choices, OS defaults, etc.).
        void prompt_user_to_save_file_async(
            std::function<void(FileDialogResponse&&)> callback,
            std::span<const FileDialogFilter> filters = {},
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt
        );
        void prompt_user_to_save_file_async(
            std::function<void(FileDialogResponse&&)> callback,
            std::initializer_list<const FileDialogFilter> filters = {},
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt)
        {
            return prompt_user_to_save_file_async(
                std::move(callback),
                std::span{filters},
                std::move(initial_directory_to_show)
            );
        }

        // Prompts a user to select a new or existing filesystem path where they would like
        // to save the file, with the option to file to have a specific extension - even if the
        // user types a filename without the extension into the dialog.
        //
        // - `callback` is called when either the user selects a file or cancels out of the dialog.
        //   If provided, the given path will always end with the specified extension. `std::nullopt`
        //   is sent through the callback when the user cancels out of the dialog.
        //
        // - `maybe_extension` can be `std::nullopt`, meaning "don't filter by extension", or a single
        //   extension (e.g. "blend").
        //
        // - `maybe_initial_directory_to_open` can be `std::nullopt`, meaning "use a system-defined default"
        //   or a directory to initially show to the user when the prompt opens.
        void prompt_user_to_save_file_with_extension_async(
            std::function<void(std::optional<std::filesystem::path>)> callback,
            std::optional<std::string_view> maybe_extension = std::nullopt,
            std::optional<std::filesystem::path> initial_directory_to_show = std::nullopt
        );

        // returns the ID of the main window
        WindowID main_window_id() const;

        // Returns the dimensions of the main application window in device-independent pixels.
        Vec2 main_window_dimensions() const;

        // Requests that the main window dimensions are set to the given dimensions in device-independent pixels.
        void try_async_set_main_window_dimensions(Vec2);

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

        // returns `true` if the main application window is minimized
        bool is_main_window_minimized() const;

        // pushes the given cursor onto the application-wide cursor stack, making it
        // the currently-active cursor until it is either popped, via `pop_cursor_override`,
        // or another cursor is pushed.
        void push_cursor_override(const Cursor&);
        void pop_cursor_override();

        // enables/disables "grabbing" the mouse cursor in the main window
        void enable_main_window_grab();
        void disable_main_window_grab();

        // if the main window is focused with the mouse, returns the current position
        // of the mouse in screen space in device-independent pixels.
        //
        // otherwise, returns `std::nullopt`.
        std::optional<Vec2> mouse_pos_in_main_window() const;

        // returns `true` if the given window has input focus
        bool has_input_focus(WindowID) const;

        // returns the ID of the window, if any, that currently has the user's keyboard focus.
        //
        // a default-constructed `WindowID` is returned if no window has keyboard focus.
        WindowID get_keyboard_focus() const;

        // sets the rectangle, defined in screen space and device-independent pixels that's,
        // used to type unicode text inputs.
        //
        // native input methods can place a window with word suggestions near the input
        // in the main window, without covering the text that's being inputted, this
        // indicates to the operating system  where the input rectangle is so that it
        // can place and operating-system-defined overlay in the correct location.
        void set_main_window_unicode_input_rect(const Rect& screen_rect);

        // start accepting unicode text input events for the given window
        //
        // it's usually necessary to call `set_unicode_input_rect` before calling this, so that
        // the text input UI is placed correctly.
        void start_text_input(WindowID);

        // stop accepting unicode text input events for the given window
        void stop_text_input(WindowID);

        // makes the main window fullscreen, but still composited with the desktop (so-called
        // 'windowed maximized' in games)
        void make_main_window_fullscreen();

        // makes the main window windowed (as opposed to fullscreen)
        void make_main_window_windowed();

        // returns the recommended number of antialiasing samples that 3D rendering
        // code should use when rendering directly to the main application window (based
        // on user settings, etc.)
        AntiAliasingLevel anti_aliasing_level() const;

        // sets the recommended number of antialiasing samples that 3D rendering code
        // should use when rendering directly to the main application window.
        //
        // throws if `samples > max_anti_aliasing_level()`
        void set_anti_aliasing_level(AntiAliasingLevel);

        // returns the maximum number of antialiasing samples that the graphics backend supports
        AntiAliasingLevel max_anti_aliasing_level() const;

        // returns true if the application is in debug mode
        //
        // other parts of the application can use this to decide whether to render
        // extra debug elements, etc.
        bool is_in_debug_mode() const;
        void set_debug_mode(bool);

        // returns true if VSYNC has been enabled in the graphics backend
        bool is_vsync_enabled() const;
        void set_vsync_enabled(bool);

        // add an annotation to the current frame with the given `label`
        // and location in screen space and device-independent pixels.
        //
        // the annotation is added to the data returned by `App::request_screenshot_of_main_window`
        void add_main_window_frame_annotation(std::string_view label, const Rect& screen_rect);

        // returns a future that asynchronously yields an annotated screenshot of the
        // next frame of the main application window
        //
        // client code can submit annotations with `App::add_frame_annotation`
        std::future<Screenshot> request_screenshot_of_main_window();

        // returns human-readable strings representing (parts of) the currently-active graphics backend (e.g. OpenGL)
        std::string graphics_backend_vendor_string() const;
        std::string graphics_backend_renderer_string() const;
        std::string graphics_backend_version_string() const;
        std::string graphics_backend_shading_language_version_string() const;

        // returns the number of times this `App` has drawn a frame to the main application
        // window.
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
        // like laptops), but top-level widgets *must* ensure the application keeps moving forward by
        // calling methods like `request_redraw` or by pumping other events into the loop.
        bool is_main_loop_waiting() const;
        void set_main_loop_waiting(bool);
        void make_main_loop_waiting();
        void make_main_loop_polling();
        void request_redraw();  // threadsafe: used to make a waiting loop redraw

        // fill all pixels in the main window with the given color
        void clear_main_window(const Color& = Color::clear());

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
        static int main_internal(const AppMetadata& metadata, const std::function<std::unique_ptr<Widget>()>& widget_ctor);

        // returns a full filesystem path to runtime resource in `resources/` dir
        std::filesystem::path get_resource_filepath(const ResourcePath&) const;

        // try and retrieve a singleton that has the same lifetime as the app
        std::shared_ptr<void> upd_singleton(const std::type_info&, const std::function<std::shared_ptr<void>()>&);

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
