#include "App.h"

#include <oscar/Graphics/GraphicsContext.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppClock.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/FilesystemResourceLoader.h>
#include <oscar/Platform/IResourceLoader.h>
#include <oscar/Platform/IScreen.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Platform/Screenshot.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/Detail/SDL2Helpers.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/FilesystemHelpers.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace sdl = osc::sdl;
using namespace osc;

namespace
{
    App* g_app_global = nullptr;

    void sdl_gl_set_attribute_or_throw(
        SDL_GLattr attribute,
        CStringView attribute_readable_name,
        int new_attribute_value,
        CStringView value_readable_name)
    {
        if (SDL_GL_SetAttribute(attribute, new_attribute_value)) {
            std::stringstream msg;
            msg << "SDL_GL_SetAttribute failed when setting " << attribute_readable_name << " = " << value_readable_name << ": " << SDL_GetError();
            throw std::runtime_error{std::move(msg).str()};
        }
    }

    // install backtrace dumper
    //
    // useful if the application fails in prod: can provide some basic backtrace
    // info that users can paste into an issue or something, which is *a lot* more
    // information than "yeah, it's broke"
    bool ensure_backtrace_handler_enabled(const std::filesystem::path& crash_dump_dir)
    {
        log_info("enabling backtrace handler");
        InstallBacktraceHandler(crash_dump_dir);
        return true;
    }

    bool configure_application_log(const AppConfig& config)
    {
        if (auto logger = defaultLogger()) {
            logger->set_level(config.getRequestedLogLevel());
        }
        return true;
    }

    // initialize the main application window
    sdl::Window create_main_app_window(const AppConfig& config, CStringView application_name)
    {
        log_info("initializing main application window");

        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_PROFILE_MASK, "SDL_GL_CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_CORE, "SDL_GL_CONTEXT_PROFILE_CORE");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MAJOR_VERSION, "SDL_GL_CONTEXT_MAJOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MINOR_VERSION, "SDL_GL_CONTEXT_MINOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_FLAGS, "SDL_GL_CONTEXT_FLAGS", SDL_GL_CONTEXT_DEBUG_FLAG, "SDL_GL_CONTEXT_DEBUG_FLAG");
        sdl_gl_set_attribute_or_throw(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "SDL_GL_FRAMEBUFFER_SRGB_CAPABLE", 1, "1");

        // careful about setting resolution, position, etc. - some people have *very* shitty
        // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
        // be <700 px high)
        constexpr int x = SDL_WINDOWPOS_CENTERED;
        constexpr int y = SDL_WINDOWPOS_CENTERED;
        constexpr int width = 800;
        constexpr int height = 600;

        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
        if (auto v = config.getValue("experimental_feature_flags/high_dpi_mode"); v and v->toBool()) {
            flags |= SDL_WINDOW_ALLOW_HIGHDPI;
            SetProcessHighDPIMode();
        }

        return sdl::CreateWindoww(application_name, x, y, width, height, flags);
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
        const auto rv = CurrentExeDir();
        log_info("executable directory: %s", rv.string().c_str());
        return rv;
    }

    // computes the user's data directory and also logs it to the console for user-facing feedback
    std::filesystem::path get_current_user_dir_and_log_it(
        CStringView organization_name,
        CStringView application_name)
    {
        const auto rv = GetUserDataDir(organization_name, application_name);
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
        explicit TypeInfoReference(const std::type_info& typeInfo) :
            m_TypeInfo{&typeInfo}
        {}

        const std::type_info& get() const { return *m_TypeInfo; }

        friend bool operator==(const TypeInfoReference& lhs, const TypeInfoReference& rhs)
        {
            return lhs.get() == rhs.get();
        }
    private:
        const std::type_info* m_TypeInfo;
    };
}

template<>
struct std::hash<TypeInfoReference> final {
    size_t operator()(const TypeInfoReference& ref) const
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
    const std::filesystem::path& executable_dir() const { return executable_dir_; }
    const std::filesystem::path& user_data_dir() const { return user_data_dir_; }

    void show(std::unique_ptr<IScreen> screen)
    {
        log_info("showing screen %s", screen->getName().c_str());

        if (screen_) {
            throw std::runtime_error{"tried to call App::show when a screen is already being shown: you should use `request_transition` instead"};
        }

        screen_ = std::move(screen);
        next_screen_.reset();

        // ensure retained screens are destroyed when exiting this guarded path
        //
        // this means callers can call .show multiple times on the same app
        const ScopeGuard g{[this]()
        {
            screen_.reset();
            next_screen_.reset();
        }};

        run_main_loop_unguarded();
    }

    void request_transition(std::unique_ptr<IScreen> screen)
    {
        next_screen_ = std::move(screen);
    }

    void request_quit()
    {
        quit_requested_ = true;
    }

    Vec2 dimensions() const
    {
        return Vec2{sdl::GetWindowSizeInPixels(main_window_.get())};
    }

    void set_show_cursor(bool v)
    {
        SDL_ShowCursor(v ? SDL_ENABLE : SDL_DISABLE);
        SDL_SetWindowGrab(main_window_.get(), v ? SDL_FALSE : SDL_TRUE);
    }

    void make_fullscreen()
    {
        SDL_SetWindowFullscreen(main_window_.get(), SDL_WINDOW_FULLSCREEN);
    }

    void make_windowed_fullscreen()
    {
        SDL_SetWindowFullscreen(main_window_.get(), SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    void make_windowed()
    {
        SDL_SetWindowFullscreen(main_window_.get(), 0);
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

    bool is_in_debug_mode() const
    {
        return graphics_context_.is_in_debug_mode();
    }

    void enable_debug_mode()
    {
        graphics_context_.enable_debug_mode();
    }

    void disable_debug_mode()
    {
        graphics_context_.disable_debug_mode();
    }

    bool is_vsync_enabled() const
    {
        return graphics_context_.is_vsync_enabled();
    }

    void set_vsync(bool v)
    {
        if (v) {
            graphics_context_.enable_vsync();
        }
        else {
            graphics_context_.disable_vsync();
        }
    }

    void enable_vsync()
    {
        graphics_context_.enable_vsync();
    }

    void disable_vsync()
    {
        graphics_context_.disable_vsync();
    }

    void add_frame_annotation(std::string_view label, Rect screen_rect)
    {
        frame_annotations_.push_back(ScreenshotAnnotation{std::string{label}, screen_rect});
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
        e.type = SDL_USEREVENT;
        num_frames_to_poll_ += 2;  // immediate rendering can require rendering 2 frames before it shows something
        SDL_PushEvent(&e);
    }

    void clear_screen(const Color& color)
    {
        graphics_context_.clear_screen(color);
    }

    void set_main_window_subtitle(std::string_view sv)
    {
        auto lock = main_window_subtitle_.lock();

        if (sv == *lock) {
            return;
        }

        *lock = sv;

        const std::string new_title = sv.empty() ?
            std::string{GetBestHumanReadableApplicationName(metadata_)} :
            (std::string{sv} + " - " + GetBestHumanReadableApplicationName(metadata_));

        SDL_SetWindowTitle(main_window_.get(), new_title.c_str());
    }

    void unset_main_window_subtitle()
    {
        set_main_window_subtitle({});
    }

    const AppConfig& get_config() const { return config_; }

    AppConfig& upd_config() { return config_; }

    ResourceLoader& upd_resource_loader()
    {
        return resource_loader_;
    }

    std::filesystem::path get_resource_filepath(const ResourcePath& rp) const
    {
        return std::filesystem::weakly_canonical(config_.getResourceDir() / rp.string());
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
        const std::function<std::shared_ptr<void>()>& ctor)
    {
        auto lock = singletons_.lock();
        const auto [it, inserted] = lock->try_emplace(TypeInfoReference{type_info}, nullptr);
        if (inserted) {
            it->second = ctor();
        }
        return it->second;
    }

    sdl::Window& upd_window() { return main_window_; }

    GraphicsContext& upd_graphics_context() { return graphics_context_; }

    void* upd_raw_opengl_context_handle_HACK()
    {
        return graphics_context_.upd_raw_opengl_context_handle_HACK();
    }

private:
    bool is_window_focused() const
    {
        return (SDL_GetWindowFlags(main_window_.get()) & SDL_WINDOW_INPUT_FOCUS) != 0u;
    }

    std::future<Texture2D> request_screenshot_texture()
    {
        return graphics_context_.request_screenshot();
    }

    // perform a screen transntion between two top-level `IScreen`s
    void transition_to_next_screen()
    {
        if (not next_screen_) {
            return;
        }

        log_info("unmounting screen %s", screen_->getName().c_str());

        try {
            screen_->onUnmount();
        }
        catch (const std::exception& ex) {
            log_error("error unmounting screen %s: %s", screen_->getName().c_str(), ex.what());
            screen_.reset();
            throw;
        }

        screen_.reset();
        screen_ = std::move(next_screen_);

        // the next screen might need to draw a couple of frames
        // to "warm up" (e.g. because it's using an immediate ui)
        num_frames_to_poll_ = 2;

        log_info("mounting screen %s", screen_->getName().c_str());
        screen_->onMount();
        log_info("transitioned main screen to %s", screen_->getName().c_str());
    }

    // the main application loop
    //
    // this is what he application enters when it `show`s the first screen
    void run_main_loop_unguarded()
    {
        // perform initial screen mount
        screen_->onMount();

        // ensure current screen is unmounted and the quitting flag is reset when
        // exiting the main loop
        const ScopeGuard on_quit_guard{[this]()
        {
            if (screen_) {
                screen_->onUnmount();
            }
            quit_requested_ = false;
        }};

        // reset counters
        perf_counter_ = SDL_GetPerformanceCounter();
        frame_counter_ = 0;
        frame_start_time_ = convert_perf_counter_to_appclock(perf_counter_, perf_counter_frequency_);
        time_since_last_frame_ = AppClock::duration{1.0f/60.0f};  // (estimated value for first frame)

        while (true) {  // game loop pattern

            // pump events
            {
                OSC_PERF("App/pumpEvents");

                bool shouldWait = is_in_wait_mode_ and num_frames_to_poll_ <= 0;
                num_frames_to_poll_ = max(0, num_frames_to_poll_ - 1);

                for (SDL_Event e; shouldWait ? SDL_WaitEventTimeout(&e, 1000) : SDL_PollEvent(&e);) {
                    shouldWait = false;

                    if (e.type == SDL_WINDOWEVENT) {
                        // window was resized and should be drawn a couple of times quickly
                        // to ensure any immediate UIs in screens are updated
                        num_frames_to_poll_ = 2;
                    }

                    // let screen handle the event
                    screen_->onEvent(e);

                    if (quit_requested_) {
                        // screen requested application quit, so exit this function
                        return;
                    }

                    if (next_screen_) {
                        // screen requested a new screen, so perform the transition
                        transition_to_next_screen();
                    }

                    if (e.type == SDL_DROPTEXT or e.type == SDL_DROPFILE) {
                        SDL_free(e.drop.file);  // SDL documentation mandates that the caller frees this
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
                OSC_PERF("App/onTick");
                screen_->onTick();
            }

            if (quit_requested_) {
                // screen requested application quit, so exit this function
                return;
            }

            if (next_screen_) {
                // screen requested a new screen, so perform the transition
                transition_to_next_screen();
                continue;
            }

            // "draw" the screen into the window framebuffer
            {
                OSC_PERF("App/onDraw");
                screen_->onDraw();
            }

            // "present" the rendered screen to the user (can block on VSYNC)
            {
                OSC_PERF("App/swap_buffers");
                graphics_context_.swap_buffers(*main_window_);
            }

            // handle annotated screenshot requests (if any)
            {
                // save this frame's annotations into the requests, if necessary
                for (AnnotatedScreenshotRequest& req : active_screenshot_requests_)
                {
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
                    [](const AnnotatedScreenshotRequest& req)
                    {
                        return not req.underlying_future.valid();
                    }
                );
            }

            // care: only update the frame counter here because the above methods
            // and checks depend on it being consistient throughout a single crank
            // of the application loop
            ++frame_counter_;

            if (quit_requested_) {
                // screen requested application quit, so exit this function
                return;
            }

            if (next_screen_) {
                // screen requested a new screen, so perform the transition
                transition_to_next_screen();
                continue;
            }
        }
    }

    // immutable application metadata (can be provided at runtime via ctor)
    AppMetadata metadata_;

    // path to the directory that the application's executable is contained within
    std::filesystem::path executable_dir_ = get_current_exe_dir_and_log_it();

    // path to the write-able user data directory
    std::filesystem::path user_data_dir_ = get_current_user_dir_and_log_it(
        metadata_.getOrganizationName(),
        metadata_.getApplicationName()
    );

    // top-level application configuration
    AppConfig config_{
        metadata_.getOrganizationName(),
        metadata_.getApplicationName()
    };

    // ensure the application log is configured according to the given configuration file
    bool log_is_configured_ = configure_application_log(config_);

    // enable the stack backtrace handler (if necessary - once per process)
    bool backtrace_handler_is_installed_ = ensure_backtrace_handler_enabled(user_data_dir_);

    // top-level runtime resource loader
    ResourceLoader resource_loader_ = make_resource_loader<FilesystemResourceLoader>(config_.getResourceDir());

    // init SDL context (windowing, etc.)
    sdl::Context sdl_context_{SDL_INIT_VIDEO};

    // init main application window
    sdl::Window main_window_ = create_main_app_window(config_, GetBestHumanReadableApplicationName(metadata_));

    // cache for the current (caller-set) window subtitle
    SynchronizedValue<std::string> main_window_subtitle_;

    // init graphics context
    GraphicsContext graphics_context_{*main_window_};

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
    SynchronizedValue<std::unordered_map<TypeInfoReference, std::shared_ptr<void>>> singletons_;

    // how many antiAliasingLevel the implementation should actually use
    AntiAliasingLevel antialiasing_level_ = min(graphics_context_.max_antialiasing_level(), config_.getNumMSXAASamples());

    // set to true if the application should quit
    bool quit_requested_ = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool is_in_wait_mode_ = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int32_t num_frames_to_poll_ = 0;

    // current screen being shown (if any)
    std::unique_ptr<IScreen> screen_;

    // the *next* screen the application should show
    std::unique_ptr<IScreen> next_screen_;

    // frame annotations made during this frame
    std::vector<ScreenshotAnnotation> frame_annotations_;

    // any active promises for an annotated frame
    std::vector<AnnotatedScreenshotRequest> active_screenshot_requests_;
};

// public API

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

const AppConfig& osc::App::config()
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

osc::App::App(App&&) noexcept = default;

App& osc::App::operator=(App&&) noexcept = default;

osc::App::~App() noexcept
{
    g_app_global = nullptr;
}

const AppMetadata& osc::App::metadata() const
{
    return impl_->metadata();
}

const std::filesystem::path& osc::App::executable_dir() const
{
    return impl_->executable_dir();
}

const std::filesystem::path& osc::App::user_data_dir() const
{
    return impl_->user_data_dir();
}

void osc::App::show(std::unique_ptr<IScreen> s)
{
    impl_->show(std::move(s));
}

void osc::App::request_transition(std::unique_ptr<IScreen> s)
{
    impl_->request_transition(std::move(s));
}

void osc::App::request_quit()
{
    impl_->request_quit();
}

Vec2 osc::App::dimensions() const
{
    return impl_->dimensions();
}

void osc::App::set_show_cursor(bool v)
{
    impl_->set_show_cursor(v);
}

void osc::App::make_fullscreen()
{
    impl_->make_fullscreen();
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

bool osc::App::is_in_debug_mode() const
{
    return impl_->is_in_debug_mode();
}

void osc::App::enable_debug_mode()
{
    impl_->enable_debug_mode();
}

void osc::App::disable_debug_mode()
{
    impl_->disable_debug_mode();
}

bool osc::App::is_vsync_enabled() const
{
    return impl_->is_vsync_enabled();
}

void osc::App::set_vsync(bool v)
{
    impl_->set_vsync(v);
}

void osc::App::enable_vsync()
{
    impl_->enable_vsync();
}

void osc::App::disable_vsync()
{
    impl_->disable_vsync();
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

void osc::App::set_main_window_subtitle(std::string_view sv)
{
    impl_->set_main_window_subtitle(sv);
}

void osc::App::unset_main_window_subtitle()
{
    impl_->unset_main_window_subtitle();
}

const AppConfig& osc::App::get_config() const
{
    return impl_->get_config();
}

AppConfig& osc::App::upd_config()
{
    return impl_->upd_config();
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
    const std::function<std::shared_ptr<void>()>& ctor)
{
    return impl_->upd_singleton(type_info, ctor);
}

SDL_Window* osc::App::upd_underlying_window()
{
    return impl_->upd_window().get();
}

void* osc::App::upd_underlying_opengl_context()
{
    return impl_->upd_graphics_context().upd_raw_opengl_context_handle_HACK();
}
