#include "App.h"

#include <oscar/Graphics/GraphicsContext.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/RectFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppClock.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Cursor.h>
#include <oscar/Platform/CursorShape.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/FilesystemResourceLoader.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Platform/Screen.h>
#include <oscar/Platform/Screenshot.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <ankerl/unordered_dense.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace osc;

template<>
struct osc::Converter<SDL_Rect, Rect> final {
    Rect operator()(const SDL_Rect& rect) const
    {
        const Vec2 top_left{rect.x, rect.y};
        const Vec2 dimensions{rect.w, rect.h};
        return {top_left, top_left + dimensions};
    }
};

namespace osc::sdl
{
    // RAII wrapper for `SDL_Init` and `SDL_Quit`
    //     https://wiki.libsdl.org/SDL_Quit
    class Context final {
    public:
        explicit Context(Uint32 flags)
        {
            if (not SDL_Init(flags)) {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(const Context&) = delete;
        Context(Context&&) noexcept = delete;
        Context& operator=(const Context&) = delete;
        Context& operator=(Context&&) noexcept = delete;
        ~Context() noexcept
        {
            SDL_Quit();
        }
    };

    // https://wiki.libsdl.org/SDL_Init
    inline Context Init(Uint32 flags)
    {
        return Context{flags};
    }

    // RAII wrapper around `SDL_Window` that calls `SDL_DestroyWindow` on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class Window final {
    public:
        explicit Window(SDL_Window * _ptr) :
            window_handle_{_ptr}
        {}
        Window(const Window&) = delete;
        Window(Window&& tmp) noexcept :
            window_handle_{std::exchange(tmp.window_handle_, nullptr)}
        {}
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) noexcept = delete;
        ~Window() noexcept
        {
            if (window_handle_) {
                SDL_DestroyWindow(window_handle_);
            }
        }

        SDL_Window* get() const { return window_handle_; }

        SDL_Window& operator*() const { return *window_handle_; }

    private:
        SDL_Window* window_handle_;
    };
}

namespace
{
    App* g_app_global = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#ifndef EMSCRIPTEN
    void sdl_gl_set_attribute_or_throw(
        SDL_GLAttr attribute,
        CStringView attribute_readable_name,
        int new_attribute_value,
        CStringView value_readable_name)
    {
        if (not SDL_GL_SetAttribute(attribute, new_attribute_value)) {
            std::stringstream msg;
            msg << "SDL_GL_SetAttribute failed when setting " << attribute_readable_name << " = " << value_readable_name << ": " << SDL_GetError();
            throw std::runtime_error{std::move(msg).str()};
        }
    }
#endif

    // install backtrace dumper
    //
    // useful if the application fails in prod: can provide some basic backtrace
    // info that users can paste into an issue or something, which is *a lot* more
    // information than "yeah, it's broke"
    bool ensure_backtrace_handler_enabled(const std::filesystem::path& crash_dump_dir)
    {
        log_info("enabling backtrace handler");
        enable_crash_signal_backtrace_handler(crash_dump_dir);
        return true;
    }

    LogLevel get_log_level_from_settings(const AppSettings& settings)
    {
        if (const auto v = settings.find_value("log_level")) {
            if (auto parsed = try_parse_as_log_level(to<std::string>(*v))) {
                return *parsed;
            }
        }
        return LogLevel::DEFAULT;
    }

    bool configure_application_log(const AppSettings& config)
    {
        if (auto logger = global_default_logger()) {
            logger->set_level(get_log_level_from_settings(config));
        }
        return true;
    }

    // initialize the main application window
    sdl::Window create_main_app_window(const AppSettings&, CStringView application_name)
    {
        log_info("initializing main application window");

#ifndef EMSCRIPTEN
        // note: cannot set context attributes in EMSCRIPTEN
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_PROFILE_MASK, "SDL_GL_CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_CORE, "SDL_GL_CONTEXT_PROFILE_CORE");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MAJOR_VERSION, "SDL_GL_CONTEXT_MAJOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MINOR_VERSION, "SDL_GL_CONTEXT_MINOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_FLAGS, "SDL_GL_CONTEXT_FLAGS", SDL_GL_CONTEXT_DEBUG_FLAG, "SDL_GL_CONTEXT_DEBUG_FLAG");
        sdl_gl_set_attribute_or_throw(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "SDL_GL_FRAMEBUFFER_SRGB_CAPABLE", 1, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.5: Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
        // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
        // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
        // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
        // you can ignore SDL_MOUSEBUTTONDOWN events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED)
#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.18: Enable native IME.
        // IMPORTANT: This is used at the time of SDL_CreateWindow() so this will only affects secondary windows, if any.
        // For the main window to be affected, your application needs to call this manually before calling SDL_CreateWindow().
#ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.22: Disable auto-capture, this is preventing drag and drop across multiple windows (see #5710)
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
        SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
#endif
        SDL_PropertiesID properties = SDL_CreateProperties();
        const ScopeGuard g{[&]{ SDL_DestroyProperties(properties); }};

        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, application_name.c_str());
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 600);

        SDL_Window* const rv = SDL_CreateWindowWithProperties(properties);
        if (rv == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return sdl::Window{rv};
    }

    AppClock::duration convert_perf_ticks_to_appclock_duration(Uint64 ticks, Uint64 frequency)
    {
        const auto dticks = static_cast<double>(ticks);
        const auto dfrequency = static_cast<double>(frequency);
        const auto duration = static_cast<AppClock::rep>(dticks/dfrequency);

        return AppClock::duration{duration};
    }

    AppClock::time_point convert_perf_counter_to_appclock(Uint64 ticks, Uint64 frequency)
    {
        return AppClock::time_point{convert_perf_ticks_to_appclock_duration(ticks, frequency)};
    }

    std::filesystem::path get_current_exe_dir_and_log_it()
    {
        auto rv = current_executable_directory();
        log_info("executable directory: %s", rv.string().c_str());
        return rv;
    }

    // computes the user's data directory and also logs it to the console for user-facing feedback
    std::filesystem::path get_current_user_dir_and_log_it(
        CStringView organization_name,
        CStringView application_name)
    {
        auto rv = user_data_directory(organization_name, application_name);
        log_info("user data directory: %s", rv.string().c_str());
        return rv;
    }
}

namespace
{
    // an "active" request for an annotated screenshot
    //
    // has a data dependency on the backend first providing a "raw" image, which
    // is then tagged with annotations
    struct AnnotatedScreenshotRequest final {

        AnnotatedScreenshotRequest(
            size_t frame_requested_,
            std::future<Texture2D> underlying_future_) :

            frame_requested{frame_requested_},
            underlying_future{std::move(underlying_future_)}
        {}

        // the frame on which the screenshot was requested
        size_t frame_requested;

        // underlying (to-be-waited-on) future for the screenshot
        std::future<Texture2D> underlying_future;

        // our promise to the caller, who is waiting for an annotated image
        std::promise<Screenshot> result_promise;

        // annotations made during the requested frame (if any)
        std::vector<ScreenshotAnnotation> annotations;
    };

    // wrapper class for storing std::type_info as a hash-able type
    class TypeInfoReference final {
    public:
        explicit TypeInfoReference(const std::type_info& type_info) :
            type_info_{&type_info}
        {}

        const std::type_info& get() const { return *type_info_; }

        friend bool operator==(const TypeInfoReference&, const TypeInfoReference&) = default;
    private:
        const std::type_info* type_info_;
    };

    // A handle to a single OS mouse cursor (that the UI may switch to at runtime).
    class SystemCursor final {
    public:
        SystemCursor() = default;

        explicit SystemCursor(SDL_SystemCursor id) :
            ptr_(SDL_CreateSystemCursor(id))
        {}

        explicit operator bool () const { return static_cast<bool>(ptr_); }

        SDL_Cursor* get() { return ptr_.get(); }
    private:
        struct CursorDeleter {
            void operator()(SDL_Cursor* ptr) { SDL_DestroyCursor(ptr); }
        };

        std::unique_ptr<SDL_Cursor, CursorDeleter> ptr_;
    };

    // A collection of all OS mouse cursors that the UI is capable of switching to.
    class SystemCursors final {
    public:
        SystemCursor& operator[](CursorShape shape) { return cursors_.at(to_index(shape)); }
    private:
        std::array<SystemCursor, num_options<CursorShape>()> cursors_ = std::to_array({
            SystemCursor(SDL_SYSTEM_CURSOR_DEFAULT),     // CursorShape::Arrow
            SystemCursor(SDL_SYSTEM_CURSOR_TEXT),     // CursorShape::IBeam
            SystemCursor(SDL_SYSTEM_CURSOR_MOVE),   // CursorShape::ResizeAll
            SystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE),    // CursorShape::ResizeVertical
            SystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE),    // CursorShape::ResizeHorizontal
            SystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE),  // CursorShape::ResizeDiagonalNESW
            SystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE),  // CursorShape::ResizeDiagonalNWSE
            SystemCursor(SDL_SYSTEM_CURSOR_POINTER),      // CursorShape::PointingHand
            SystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED),        // CursorShape::Forbidden
            SystemCursor{},                            // CursorShape::Hidden
        });
    };

    class CursorHandler final {
    public:
        CursorHandler()
        {
            push_cursor_override(Cursor{CursorShape::Forbidden});  // initialize sentinel
        }
        CursorHandler(const CursorHandler&) = delete;
        CursorHandler(CursorHandler&&) = delete;
        CursorHandler& operator=(const CursorHandler&) = delete;
        CursorHandler& operator=(CursorHandler&&) = delete;
        ~CursorHandler() noexcept
        {
            // try to reset the cursor to default
            if (cursor_stack_.size() > 1) {
                SDL_ShowCursor();
                SDL_SetCursor(system_mouse_cursors_[CursorShape::Arrow].get());
            }
        }

        void push_cursor_override(const Cursor& cursor)
        {
            if (cursor.shape() != CursorShape::Hidden) {
                SDL_ShowCursor();
            }
            else {
                SDL_HideCursor();
            }
            SDL_SetCursor(system_mouse_cursors_[cursor.shape()].get());
            cursor_stack_.push_back(cursor.shape());
        }

        void pop_cursor_override()
        {
            // note: there's a sentinel cursor at the bottom of the stack that's initialized
            //       by the constructor
            OSC_ASSERT(cursor_stack_.size() > 1 && "tried to call App::pop_cursor_override when no cursor overrides were pushed");

            cursor_stack_.pop_back();
            SDL_SetCursor(system_mouse_cursors_[cursor_stack_.back()].get());
            if (cursor_stack_.empty() or cursor_stack_.back() != CursorShape::Hidden) {
                SDL_ShowCursor();
            }
            else {
                SDL_HideCursor();
            }
        }
    private:
        // runtime lookup of all available mouse cursors
        SystemCursors system_mouse_cursors_;

        // current stack of application-level cursor overrides
        std::vector<CursorShape> cursor_stack_;
    };
}

template<>
struct std::hash<TypeInfoReference> final {
    size_t operator()(const TypeInfoReference& ref) const noexcept
    {
        return ref.get().hash_code();
    }
};

// main application state
//
// this is what "booting the application" actually initializes
class osc::App::Impl final {
public:
    explicit Impl(const AppMetadata& metadata) :  // NOLINT(modernize-pass-by-value)
        metadata_{metadata}
    {}

    const AppMetadata& metadata() const { return metadata_; }
    const std::filesystem::path& executable_directory() const { return executable_dir_; }
    const std::filesystem::path& user_data_directory() const { return user_data_dir_; }

    void setup_main_loop(std::unique_ptr<Screen> screen)
    {
        if (screen_) {
            throw std::runtime_error{"tried to call `App::setup_main_loop` when a screen is already being shown (and, therefore, `App::teardown_main_loop` wasn't called). If you want to change the applications screen from *within* some other screen, call `request_transition` instead"};
        }

        log_info("initializing application main loop with screen %s", screen->name().c_str());

        // reset loop-dependent state variables
        perf_counter_ = SDL_GetPerformanceCounter();
        frame_counter_ = 0;
        frame_start_time_ = convert_perf_counter_to_appclock(perf_counter_, perf_counter_frequency_);
        time_since_last_frame_ = AppClock::duration{1.0f/60.0f};  // (dummy value for the first frame)
        quit_requested_ = false;
        is_in_wait_mode_ = false;
        num_frames_to_poll_ = 2;

        // perform initial screen mount
        screen_ = std::move(screen);
        screen_->on_mount();
    }

    AppMainLoopStatus do_main_loop_step()
    {
        // pump events
        {
            OSC_PERF("App/pump_events");

            bool should_wait = is_in_wait_mode_ and num_frames_to_poll_ <= 0;
            num_frames_to_poll_ = max(0, num_frames_to_poll_ - 1);

            for (SDL_Event e; should_wait ? SDL_WaitEventTimeout(&e, 1000) : SDL_PollEvent(&e);) {
                should_wait = false;

                // edge-case: it's an `SDL_USEREVENT`, which should only propagate from this
                // compilation unit, and is always either blank (`data1 == nullptr`) or has
                // two pointers: a not-owned `Widget*` receiver and an owned `Event*`.
                if (e.type == SDL_EVENT_USER) {
                    if (e.user.data1) {
                        // it's an application-enacted (i.e. not spontaneous, OS-enacted, etc.) event
                        // that should be immediately dispatched.
                        auto* receiver = static_cast<Widget*>(e.user.data1);
                        auto  event = std::unique_ptr<Event>(static_cast<Event*>(e.user.data2));
                        notify(*receiver, *event);
                        continue;  // event handled - go get the next one
                    }
                    else {
                        // it's a blank user event from `request_redraw` that's being used to wake
                        // up the event loop
                        continue;  // handled - it woke up the event loop
                    }
                }

                // let screen handle the event
                bool screen_handled_event = false;
                if (auto parsed = try_parse_into_event(e)) {
                    screen_handled_event = screen_->on_event(*parsed);
                }

                // if the active screen didn't handle the event, try to handle it here by following
                // reasonable heuristics
                if (not screen_handled_event) {
                    if (SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST) {
                        // window was resized and should be drawn a couple of times quickly
                        // to ensure any immediate UIs in screens are updated
                        num_frames_to_poll_ = 2;
                    }
                    else if (e.type == SDL_EVENT_QUIT) {
                        request_quit();  // i.e. "as if the current screen tried to quit"
                    }
                }

                if (std::exchange(quit_requested_, false)) {
                    // screen requested that the application quits, so propagate this upwards
                    return AppMainLoopStatus::quit_requested();
                }

                if (next_screen_) {
                    // screen requested a new screen, so perform the transition
                    transition_to_next_screen();
                }
            }
        }

        // update clocks
        {
            const auto counter = SDL_GetPerformanceCounter();
            const Uint64 delta_ticks = counter - perf_counter_;

            perf_counter_ = counter;
            frame_start_time_ = convert_perf_counter_to_appclock(counter, perf_counter_frequency_);
            time_since_last_frame_ = convert_perf_ticks_to_appclock_duration(delta_ticks, perf_counter_frequency_);
        }

        // "tick" the screen
        {
            OSC_PERF("App/on_tick");
            screen_->on_tick();
        }

        if (std::exchange(quit_requested_, false)) {
            // screen requested that the application quits, so propagate this upwards
            return AppMainLoopStatus::quit_requested();
        }

        if (next_screen_) {
            // screen requested a new screen, so perform the transition
            transition_to_next_screen();
            return AppMainLoopStatus::ok();
        }

        // "draw" the screen into the window framebuffer
        {
            OSC_PERF("App/on_draw");
            screen_->on_draw();
        }

        // "present" the rendered screen to the user (can block on VSYNC)
        {
            OSC_PERF("App/swap_buffers");
            graphics_context_.swap_buffers(*main_window_);
        }

        // handle annotated screenshot requests (if any)
        handle_screenshot_requests_for_this_frame();

        // care: only update the frame counter here because the above methods
        // and checks depend on it being consistent throughout a single crank
        // of the application loop
        ++frame_counter_;

        if (std::exchange(quit_requested_, false)) {
            // screen requested that the application quits, so propagate this upwards
            return AppMainLoopStatus::quit_requested();
        }

        if (next_screen_) {
            // screen requested a new screen, so perform the transition
            transition_to_next_screen();
        }

        return AppMainLoopStatus::ok();
    }

    void teardown_main_loop()
    {
        if (screen_) {
            screen_->on_unmount();
            screen_.reset();
        }
        next_screen_.reset();

        frame_annotations_.clear();
        active_screenshot_requests_.clear();
    }

    void post_event(Widget& receiver, std::unique_ptr<Event> event)
    {
        SDL_Event e{};
        e.type = SDL_EVENT_USER;
        e.user.data1 = &receiver;
        e.user.data2 = event.release();
        SDL_PushEvent(&e);
    }

    bool notify(Widget& receiver, Event& event)
    {
        Widget* current = &receiver;
        do {
            if (current->on_event(event)) {
                return true;
            }
            current = current->parent();
        }
        while (current != nullptr and event.propagates());

        return false;
    }

    void show(std::unique_ptr<Screen> screen)
    {
        setup_main_loop(std::move(screen));

        // ensure `teardown_main_loop` is called - even if there's an exception
        const ScopeGuard scope_guard{[this]() { teardown_main_loop(); }};

        while (do_main_loop_step()) {
            ;  // keep ticking the loop until it's not ok
        }
    }

    void request_transition(std::unique_ptr<Screen> screen)
    {
        next_screen_ = std::move(screen);
    }

    void request_quit()
    {
        quit_requested_ = true;
    }

    std::vector<Monitor> monitors() const
    {
        std::vector<Monitor> rv;

        int display_count = 0;
        SDL_DisplayID* const first_display = SDL_GetDisplays(&display_count);
        if (first_display == nullptr) {
            const char* error = SDL_GetError();
            std::stringstream msg;
            msg << "SDL_GetDisplays: error: " << error;
            throw std::runtime_error{std::move(msg).str()};
        }
        const ScopeGuard displays_deleter{[first_display]{ SDL_free(first_display); }};
        const std::span<const SDL_DisplayID> display_ids{first_display, static_cast<size_t>(display_count)};

        rv.reserve(display_count);
        for (const SDL_DisplayID display_id : display_ids) {
            SDL_Rect display_bounds;
            SDL_GetDisplayBounds(display_id, &display_bounds);


#if SDL_HAS_USABLE_DISPLAY_BOUNDS
            SDL_Rect usable_bounds;
            SDL_GetDisplayUsableBounds(display_id, &usable_bounds);
#else
            const SDL_Rect usable_bounds = display_bounds;
#endif
            const float dpi = SDL_GetDisplayContentScale(display_id) * 96.0f;
            rv.emplace_back(to<Rect>(display_bounds), to<Rect>(usable_bounds), dpi);
        }

        return rv;
    }

    Vec2 main_window_dimensions() const
    {
        return main_window_pixel_dimensions() / main_window_device_pixel_ratio();
    }

    Vec2 main_window_pixel_dimensions() const
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(main_window_.get(), &w, &h);
        return Vec2{static_cast<float>(w), static_cast<float>(h)};
    }

    float main_window_device_pixel_ratio() const
    {
        return SDL_GetWindowDisplayScale(main_window_.get());
    }

    float os_to_main_window_device_independent_ratio() const
    {
        // i.e. scale the event by multiplying it by the pixel density (yielding a
        // pixel-based event value) and then dividing it by the suggested window
        // display scale (yielding a device-independent pixel value).
        return SDL_GetWindowPixelDensity(main_window_.get()) / SDL_GetWindowDisplayScale(main_window_.get());
    }

    float main_window_device_independent_to_os_ratio() const
    {
        return 1.0f / os_to_main_window_device_independent_ratio();
    }

    bool is_main_window_minimized() const
    {
        return (SDL_GetWindowFlags(main_window_.get()) & SDL_WINDOW_MINIMIZED) != 0u;
    }

    void push_cursor_override(const Cursor& cursor)
    {
        cursor_handler_.push_cursor_override(cursor);
    }

    void pop_cursor_override()
    {
        cursor_handler_.pop_cursor_override();
    }

    void enable_main_window_grab()
    {
        SDL_SetWindowMouseGrab(main_window_.get(), true);
    }

    void disable_main_window_grab()
    {
        SDL_SetWindowMouseGrab(main_window_.get(), false);
    }

    void set_unicode_input_rect(const Rect& rect)
    {
        const float device_independent_to_sdl3_ratio = main_window_device_independent_to_os_ratio();

        const SDL_Rect r{
            .x = static_cast<int>(device_independent_to_sdl3_ratio * rect.p1.x),
            .y = static_cast<int>(device_independent_to_sdl3_ratio * rect.p1.y),
            .w = static_cast<int>(device_independent_to_sdl3_ratio * dimensions_of(rect).x),
            .h = static_cast<int>(device_independent_to_sdl3_ratio * dimensions_of(rect).y),
        };
        SDL_SetTextInputArea(main_window_.get(), &r, 0);
    }

    void set_show_cursor(bool v)
    {
        if (v) {
            SDL_ShowCursor();
        }
        else {
            SDL_HideCursor();
        }
        SDL_SetWindowMouseGrab(main_window_.get(), not v);
    }

    void make_windowed_fullscreen()
    {
        SDL_SetWindowFullscreenMode(main_window_.get(), nullptr);
        SDL_SetWindowFullscreen(main_window_.get(), true);
    }

    void make_windowed()
    {
        SDL_SetWindowFullscreen(main_window_.get(), false);
    }

    AntiAliasingLevel anti_aliasing_level() const
    {
        return antialiasing_level_;
    }

    void set_anti_aliasing_level(AntiAliasingLevel s)
    {
        antialiasing_level_ = clamp(s, AntiAliasingLevel{1}, max_anti_aliasing_level());
    }

    AntiAliasingLevel max_anti_aliasing_level() const
    {
        return graphics_context_.max_antialiasing_level();
    }

    bool is_main_window_gamma_corrected() const
    {
#ifdef EMSCRIPTEN
        return false;
#else
        return true;
#endif
    }

    bool is_in_debug_mode() const
    {
        return graphics_context_.is_in_debug_mode();
    }

    void set_debug_mode(bool v)
    {
        graphics_context_.set_debug_mode(v);
    }

    bool is_vsync_enabled() const
    {
        return graphics_context_.is_vsync_enabled();
    }

    void set_vsync_enabled(bool v)
    {
        graphics_context_.set_vsync_enabled(v);
    }

    void add_frame_annotation(std::string_view label, Rect screen_rect)
    {
        frame_annotations_.emplace_back(std::string{label}, screen_rect);
    }

    std::future<Screenshot> request_screenshot()
    {
        AnnotatedScreenshotRequest& req = active_screenshot_requests_.emplace_back(frame_counter_, request_screenshot_texture());
        return req.result_promise.get_future();
    }

    std::string graphics_backend_vendor_string() const
    {
        return graphics_context_.backend_vendor_string();
    }

    std::string graphics_backend_renderer_string() const
    {
        return graphics_context_.backend_renderer_string();
    }

    std::string graphics_backend_version_string() const
    {
        return graphics_context_.backend_version_string();
    }

    std::string graphics_backend_shading_language_version_string() const
    {
        return graphics_context_.backend_shading_language_version_string();
    }

    size_t num_frames_drawn() const
    {
        return frame_counter_;
    }

    AppClock::time_point startup_time() const
    {
        return startup_time_;
    }

    AppClock::duration frame_delta_since_startup() const
    {
        return frame_start_time_ - startup_time_;
    }

    AppClock::time_point frame_start_time() const
    {
        return frame_start_time_;
    }

    AppClock::duration frame_delta_since_last_frame() const
    {
        return time_since_last_frame_;
    }

    bool is_main_loop_waiting() const
    {
        return is_in_wait_mode_;
    }

    void set_main_loop_waiting(bool v)
    {
        is_in_wait_mode_ = v;
        request_redraw();
    }

    void make_main_loop_waiting()
    {
        set_main_loop_waiting(true);
    }

    void make_main_loop_polling()
    {
        set_main_loop_waiting(false);
    }

    void request_redraw()
    {
        SDL_Event e{};
        e.type = SDL_EVENT_USER;
        num_frames_to_poll_ += 2;  // immediate rendering can require rendering 2 frames before it shows something
        SDL_PushEvent(&e);
    }

    void clear_screen(const Color& color)
    {
        graphics_context_.clear_screen(color);
    }

    void set_main_window_subtitle(std::string_view subtitle)
    {
        auto title_lock = main_window_subtitle_.lock();

        if (subtitle == *title_lock) {
            return;
        }

        *title_lock = subtitle;

        const std::string new_title = subtitle.empty() ?
            std::string{calc_human_readable_application_name(metadata_)} :
            (std::string{subtitle} + " - " + calc_human_readable_application_name(metadata_));

        SDL_SetWindowTitle(main_window_.get(), new_title.c_str());
    }

    void unset_main_window_subtitle()
    {
        set_main_window_subtitle({});
    }

    const AppSettings& get_config() const { return config_; }

    AppSettings& upd_settings() { return config_; }

    ResourceLoader& upd_resource_loader()
    {
        return resource_loader_;
    }

    std::filesystem::path get_resource_filepath(const ResourcePath& rp) const
    {
        return std::filesystem::weakly_canonical(resources_dir_ / rp.string());
    }

    std::string slurp_resource(const ResourcePath& rp)
    {
        return resource_loader_.slurp(rp);
    }

    ResourceStream load_resource(const ResourcePath& rp)
    {
        return resource_loader_.open(rp);
    }

    std::shared_ptr<void> upd_singleton(
        const std::type_info& type_info,
        const std::function<std::shared_ptr<void>()>& singleton_constructor)
    {
        auto lock = singletons_.lock();
        const auto [it, inserted] = lock->try_emplace(TypeInfoReference{type_info}, nullptr);
        if (inserted) {
            it->second = singleton_constructor();
        }
        return it->second;
    }

    sdl::Window& upd_window() { return main_window_; }

    GraphicsContext& upd_graphics_context() { return graphics_context_; }

private:
    bool is_window_focused() const
    {
        return (SDL_GetWindowFlags(main_window_.get()) & SDL_WINDOW_INPUT_FOCUS) != 0u;
    }

    std::future<Texture2D> request_screenshot_texture()
    {
        return graphics_context_.request_screenshot();
    }

    // perform a screen transition between two top-level `Screen`s
    void transition_to_next_screen()
    {
        if (not next_screen_) {
            return;
        }

        if (screen_) {
            log_info("unmounting screen %s", screen_->name().c_str());

            try {
                screen_->on_unmount();
            }
            catch (const std::exception& ex) {
                log_error("error unmounting screen %s: %s", screen_->name().c_str(), ex.what());
                screen_.reset();
                throw;
            }

            screen_.reset();
        }

        screen_ = std::move(next_screen_);

        // the next screen might need to draw a couple of frames
        // to "warm up" (e.g. because it's using an immediate ui)
        num_frames_to_poll_ = 2;

        log_info("mounting screen %s", screen_->name().c_str());
        screen_->on_mount();
    }

    // tries to handle any active (asynchronous) screenshot requests
    void handle_screenshot_requests_for_this_frame()
    {
        // save this frame's annotations into the requests, if necessary
        for (AnnotatedScreenshotRequest& req : active_screenshot_requests_) {
            if (req.frame_requested == frame_counter_) {
                req.annotations = frame_annotations_;
            }
        }
        frame_annotations_.clear();  // this frame's annotations are now saved (if necessary)

        // complete any requests for which screenshot data has arrived
        for (AnnotatedScreenshotRequest& req : active_screenshot_requests_) {

            if (req.underlying_future.valid() and
                req.underlying_future.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {

                // screenshot is ready: create an annotated screenshot and send it to
                // the caller
                req.result_promise.set_value(Screenshot{req.underlying_future.get(), std::move(req.annotations)});
            }
        }

        // gc any invalid (i.e. handled) requests
        std::erase_if(
            active_screenshot_requests_,
            [](const AnnotatedScreenshotRequest& request)
            {
                return not request.underlying_future.valid();
            }
        );
    }

    // immutable application metadata (can be provided at runtime via ctor)
    AppMetadata metadata_;

    // top-level application configuration
    AppSettings config_{
        metadata_.organization_name(),
        metadata_.application_name()
    };

    // initialization-time resources dir (so that it doesn't have to be fetched
    // from the settings over-and-over)
    std::filesystem::path resources_dir_ = get_resource_dir_from_settings(config_);

    // path to the directory that the application's executable is contained within
    std::filesystem::path executable_dir_ = get_current_exe_dir_and_log_it();

    // path to the write-able user data directory
    std::filesystem::path user_data_dir_ = get_current_user_dir_and_log_it(
        metadata_.organization_name(),
        metadata_.application_name()
    );

    // ensures that the global application log is configured according to the
    // application's configuration file
    bool log_is_configured_ = configure_application_log(config_);

    // enable the stack backtrace handler (if necessary - once per process)
    bool backtrace_handler_is_installed_ = ensure_backtrace_handler_enabled(user_data_dir_);

    // top-level runtime resource loader
    ResourceLoader resource_loader_ = make_resource_loader<FilesystemResourceLoader>(resources_dir_);

    // SDL context (windowing, video driver, etc.)
    sdl::Context sdl_context_{SDL_INIT_VIDEO};

    // SDL main application window
    sdl::Window main_window_ = create_main_app_window(config_, calc_human_readable_application_name(metadata_));

    // cache for the current (caller-set) window subtitle
    SynchronizedValue<std::string> main_window_subtitle_;

    // 3D graphics context for the oscar graphics API
    GraphicsContext graphics_context_{*main_window_};

    // application-wide handler for the mouse cursor
    CursorHandler cursor_handler_;

    // get performance counter frequency (for the delta clocks)
    Uint64 perf_counter_frequency_ = SDL_GetPerformanceFrequency();

    // current performance counter value (recorded once per frame)
    Uint64 perf_counter_ = 0;

    // number of frames the application has drawn
    size_t frame_counter_ = 0;

    // when the application started up (set now)
    AppClock::time_point startup_time_ = convert_perf_counter_to_appclock(SDL_GetPerformanceCounter(), perf_counter_frequency_);

    // when the current frame started (set each frame)
    AppClock::time_point frame_start_time_ = startup_time_;

    // time since the frame before the current frame (set each frame)
    AppClock::duration time_since_last_frame_ = {};

    // global cache of application-wide singletons (usually, for caching)
    SynchronizedValue<ankerl::unordered_dense::map<TypeInfoReference, std::shared_ptr<void>>> singletons_;

    // how many antiAliasingLevel the implementation should actually use
    AntiAliasingLevel antialiasing_level_ = min(graphics_context_.max_antialiasing_level(), AntiAliasingLevel{4});

    // set to true if the application should quit
    bool quit_requested_ = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool is_in_wait_mode_ = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int32_t num_frames_to_poll_ = 0;

    // current screen being shown (if any)
    std::unique_ptr<Screen> screen_;

    // the *next* screen the application should show
    std::unique_ptr<Screen> next_screen_;

    // frame annotations made during this frame
    std::vector<ScreenshotAnnotation> frame_annotations_;

    // any active promises for an annotated frame
    std::vector<AnnotatedScreenshotRequest> active_screenshot_requests_;
};

App& osc::App::upd()
{
    OSC_ASSERT(g_app_global && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_app_global;
}

const App& osc::App::get()
{
    OSC_ASSERT(g_app_global && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_app_global;
}

const AppSettings& osc::App::settings()
{
    return get().get_config();
}

std::filesystem::path osc::App::resource_filepath(const ResourcePath& rp)
{
    return get().get_resource_filepath(rp);
}

std::string osc::App::slurp(const ResourcePath& rp)
{
    return upd().slurp_resource(rp);
}

ResourceStream osc::App::load_resource(const ResourcePath& rp)
{
    return upd().go_load_resource(rp);
}

ResourceLoader& osc::App::resource_loader()
{
    return upd().upd_resource_loader();
}

osc::App::App() :
    App{AppMetadata{}}
{}

osc::App::App(const AppMetadata& metadata)
{
    OSC_ASSERT(g_app_global == nullptr && "cannot instantiate multiple `App` instances at the same time");

    impl_ = std::make_unique<Impl>(metadata);
    g_app_global = this;
}

osc::App::~App() noexcept
{
    g_app_global = nullptr;
}

const AppMetadata& osc::App::metadata() const
{
    return impl_->metadata();
}

const std::filesystem::path& osc::App::executable_directory() const
{
    return impl_->executable_directory();
}

const std::filesystem::path& osc::App::user_data_directory() const
{
    return impl_->user_data_directory();
}

void osc::App::setup_main_loop(std::unique_ptr<Screen> screen)
{
    impl_->setup_main_loop(std::move(screen));
}

AppMainLoopStatus osc::App::do_main_loop_step()
{
    return impl_->do_main_loop_step();
}

void osc::App::teardown_main_loop()
{
    impl_->teardown_main_loop();
}

void osc::App::post_event(Widget& receiver, std::unique_ptr<Event> event)
{
    upd().impl_->post_event(receiver, std::move(event));
}

bool osc::App::notify(Widget& receiver, Event& event)
{
    return upd().impl_->notify(receiver, event);
}

void osc::App::show(std::unique_ptr<Screen> s)
{
    impl_->show(std::move(s));
}

void osc::App::request_transition(std::unique_ptr<Screen> s)
{
    impl_->request_transition(std::move(s));
}

void osc::App::request_quit()
{
    impl_->request_quit();
}

std::vector<Monitor> osc::App::monitors() const
{
    return impl_->monitors();
}

Vec2 osc::App::main_window_dimensions() const
{
    return impl_->main_window_dimensions();
}

Vec2 osc::App::main_window_pixel_dimensions() const
{
    return impl_->main_window_pixel_dimensions();
}

float osc::App::main_window_device_pixel_ratio() const
{
    return impl_->main_window_device_pixel_ratio();
}

float osc::App::os_to_main_window_device_independent_ratio() const
{
    return impl_->os_to_main_window_device_independent_ratio();
}

float osc::App::main_window_device_independent_to_os_ratio() const
{
    return impl_->main_window_device_independent_to_os_ratio();
}

bool osc::App::is_main_window_minimized() const
{
    return impl_->is_main_window_minimized();
}

void osc::App::push_cursor_override(const Cursor& cursor)
{
    impl_->push_cursor_override(cursor);
}

void osc::App::pop_cursor_override()
{
    impl_->pop_cursor_override();
}

void osc::App::enable_main_window_grab()
{
    impl_->enable_main_window_grab();
}

void osc::App::disable_main_window_grab()
{
    impl_->disable_main_window_grab();
}

void osc::App::set_unicode_input_rect(const Rect& rect)
{
    impl_->set_unicode_input_rect(rect);
}

void osc::App::make_windowed_fullscreen()
{
    impl_->make_windowed_fullscreen();
}

void osc::App::make_windowed()
{
    impl_->make_windowed();
}

AntiAliasingLevel osc::App::anti_aliasing_level() const
{
    return impl_->anti_aliasing_level();
}

void osc::App::set_anti_aliasing_level(AntiAliasingLevel s)
{
    impl_->set_anti_aliasing_level(s);
}

AntiAliasingLevel osc::App::max_anti_aliasing_level() const
{
    return impl_->max_anti_aliasing_level();
}

bool osc::App::is_main_window_gamma_corrected() const
{
    return impl_->is_main_window_gamma_corrected();
}

bool osc::App::is_in_debug_mode() const
{
    return impl_->is_in_debug_mode();
}

void osc::App::set_debug_mode(bool v)
{
    impl_->set_debug_mode(v);
}

bool osc::App::is_vsync_enabled() const
{
    return impl_->is_vsync_enabled();
}

void osc::App::set_vsync_enabled(bool v)
{
    impl_->set_vsync_enabled(v);
}

void osc::App::add_frame_annotation(std::string_view label, Rect screen_rect)
{
    impl_->add_frame_annotation(label, screen_rect);
}

std::future<Screenshot> osc::App::request_screenshot()
{
    return impl_->request_screenshot();
}

std::string osc::App::graphics_backend_vendor_string() const
{
    return impl_->graphics_backend_vendor_string();
}

std::string osc::App::graphics_backend_renderer_string() const
{
    return impl_->graphics_backend_renderer_string();
}

std::string osc::App::graphics_backend_version_string() const
{
    return impl_->graphics_backend_version_string();
}

std::string osc::App::graphics_backend_shading_language_version_string() const
{
    return impl_->graphics_backend_shading_language_version_string();
}

size_t osc::App::num_frames_drawn() const
{
    return impl_->num_frames_drawn();
}

AppClock::time_point osc::App::startup_time() const
{
    return impl_->startup_time();
}

AppClock::duration osc::App::frame_delta_since_startup() const
{
    return impl_->frame_delta_since_startup();
}

AppClock::time_point osc::App::frame_start_time() const
{
    return impl_->frame_start_time();
}

AppClock::duration osc::App::frame_delta_since_last_frame() const
{
    return impl_->frame_delta_since_last_frame();
}

bool osc::App::is_main_loop_waiting() const
{
    return impl_->is_main_loop_waiting();
}

void osc::App::set_main_loop_waiting(bool v)
{
    impl_->set_main_loop_waiting(v);
}

void osc::App::make_main_loop_waiting()
{
    impl_->make_main_loop_waiting();
}

void osc::App::make_main_loop_polling()
{
    impl_->make_main_loop_polling();
}

void osc::App::request_redraw()
{
    impl_->request_redraw();
}

void osc::App::clear_screen(const Color& color)
{
    impl_->clear_screen(color);
}

void osc::App::set_main_window_subtitle(std::string_view subtitle)
{
    impl_->set_main_window_subtitle(subtitle);
}

void osc::App::unset_main_window_subtitle()
{
    impl_->unset_main_window_subtitle();
}

const AppSettings& osc::App::get_config() const
{
    return impl_->get_config();
}

AppSettings& osc::App::upd_settings()
{
    return impl_->upd_settings();
}

ResourceLoader& osc::App::upd_resource_loader()
{
    return impl_->upd_resource_loader();
}

std::filesystem::path osc::App::get_resource_filepath(const ResourcePath& rp) const
{
    return impl_->get_resource_filepath(rp);
}

std::string osc::App::slurp_resource(const ResourcePath& rp)
{
    return impl_->slurp_resource(rp);
}

ResourceStream osc::App::go_load_resource(const ResourcePath& rp)
{
    return impl_->load_resource(rp);
}

std::shared_ptr<void> osc::App::upd_singleton(
    const std::type_info& type_info,
    const std::function<std::shared_ptr<void>()>& singleton_constructor)
{
    return impl_->upd_singleton(type_info, singleton_constructor);
}

SDL_Window* osc::App::upd_underlying_window()
{
    return impl_->upd_window().get();
}
