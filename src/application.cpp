#include "application.hpp"

#include "osmv_config.hpp"
#include "src/3d/gl.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/screens/error_screen.hpp"
#include "src/screens/screen.hpp"
#include "src/utils/bitwise_algs.hpp"
#include "src/utils/os.hpp"
#include "src/utils/sdl_wrapper.hpp"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/imgui.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// macros
#define OSC_SDL_GL_SetAttribute_CHECK(attr, value)                                                                     \
    {                                                                                                                  \
        int rv = SDL_GL_SetAttribute((attr), (value));                                                                 \
        if (rv != 0) {                                                                                                 \
            throw std::runtime_error{"SDL_GL_SetAttribute failed when setting " #attr " = " #value " : "s +            \
                                     SDL_GetError()};                                                                  \
        }                                                                                                              \
    }

using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
using namespace osc;

// globals
osc::Application* osc::Application::gCurrent = nullptr;

struct ImGuiContext;

namespace igx {
    struct Context final {
        std::string default_ini = osc::config::resource_path("imgui_base_config.ini").string();
        std::string user_ini = (osc::user_data_dir() / "imgui.ini").string();

        ImGuiContext* handle;

        Context() : handle{ImGui::CreateContext()} {
            configure_context(ImGui::GetIO());
            ImGui::LoadIniSettingsFromDisk(default_ini.c_str());
            ImGui::LoadIniSettingsFromDisk(user_ini.c_str());
            ImGui::GetIO().IniFilename = user_ini.c_str();
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;

        void configure_context(ImGuiIO& io) {
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            if (osc::config::should_use_multi_viewport()) {
                io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }
        }

        void reset() {
            ImGui::DestroyContext(handle);
            handle = ImGui::CreateContext();
            configure_context(ImGui::GetIO());
            ImGui::LoadIniSettingsFromDisk(default_ini.c_str());
            ImGui::LoadIniSettingsFromDisk(user_ini.c_str());
            ImGui::GetIO().IniFilename = user_ini.c_str();
        }

        ~Context() noexcept {
            ImGui::DestroyContext(handle);
        }
    };

    struct SDL2_Context final {
        SDL2_Context(SDL_Window* w, void* gl) {
            ImGui_ImplSDL2_InitForOpenGL(w, gl);
        }
        SDL2_Context(SDL2_Context const&) = delete;
        SDL2_Context(SDL2_Context&&) = delete;
        SDL2_Context& operator=(SDL2_Context const&) = delete;
        SDL2_Context& operator=(SDL2_Context&&) = delete;

        void reset(SDL_Window* w, void* gl) {
            ImGui_ImplSDL2_Shutdown();
            ImGui_ImplSDL2_InitForOpenGL(w, gl);
        }

        ~SDL2_Context() noexcept {
            ImGui_ImplSDL2_Shutdown();
        }
    };

    struct OpenGL3_Context final {
        OpenGL3_Context(char const* version) {
            ImGui_ImplOpenGL3_Init(version);
        }
        OpenGL3_Context(OpenGL3_Context const&) = delete;
        OpenGL3_Context(OpenGL3_Context&&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context const&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context&&) = delete;

        void reset(char const* version) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplOpenGL3_Init(version);
        }

        ~OpenGL3_Context() noexcept {
            ImGui_ImplOpenGL3_Shutdown();
        }
    };
}

static std::atomic<bool> g_ThrowOnOpenGLErrors = true;

// callback function suitable for glDebugMessageCallback
static void glOnDebugMessage(
    GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei, const char* message, const void*) {

    // ignore non-significant error/warning codes
    // if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    //    return;

    log::level::Level_enum lvl = log::level::debug;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        lvl = log::level::err;
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        lvl = log::level::warn;
        break;
    case GL_DEBUG_SEVERITY_LOW:
        lvl = log::level::info;
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        lvl = log::level::trace;
        break;
    }

    log::log(lvl, "OpenGL debug message:");
    log::log(lvl, "    id = %u");
    log::log(lvl, "    message = %s", message);

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_API");
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_WINDOW_SYSTEM");
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_SHADER_COMPILER");
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_THIRD_PARTY");
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_APPLICATION");
        break;
    case GL_DEBUG_SOURCE_OTHER:
        log::log(lvl, "    source = GL_DEBUG_SOURCE_OTHER");
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        log::log(lvl, "    type = GL_DEBUG_TYPE_ERROR");
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        log::log(lvl, "    type = GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        log::log(lvl, "    type = GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        log::log(lvl, "    type = GL_DEBUG_TYPE_PORTABILITY");
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        log::log(lvl, "    type = GL_DEBUG_TYPE_PERFORMANCE");
        break;
    case GL_DEBUG_TYPE_MARKER:
        log::log(lvl, "    type = GL_DEBUG_TYPE_MARKER");
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        log::log(lvl, "    type = GL_DEBUG_TYPE_PUSH_GROUP");
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        log::log(lvl, "    type = GL_DEBUG_TYPE_POP_GROUP");
        break;
    case GL_DEBUG_TYPE_OTHER:
        log::log(lvl, "    type = GL_DEBUG_TYPE_OTHER");
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        log::log(lvl, "    severity = GL_DEBUG_SEVERITY_HIGH");
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        log::log(lvl, "    severity = GL_DEBUG_SEVERITY_MEDIUM");
        break;
    case GL_DEBUG_SEVERITY_LOW:
        log::log(lvl, "    severity = GL_DEBUG_SEVERITY_LOW");
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        log::log(lvl, "    severity = GL_DEBUG_SEVERITY_NOTIFICATION");
        break;
    }

    // only throw if application is configured to do so
    if (!g_ThrowOnOpenGLErrors) {
        return;
    }

    // only throw on errors
    if (type != GL_DEBUG_TYPE_ERROR) {
        return;
    }

    // only throw on errors
    if (severity != GL_DEBUG_SEVERITY_MEDIUM && severity != GL_DEBUG_SEVERITY_HIGH) {
        return;
    }

    // if an error is about to be thrown, dump a stacktrace into the logs so that
    // downstream devs/users can potentially diagnose what's happening
    osc::write_backtrace_to_log(osc::log::level::err);

    // throw the exception from this thread
    //
    // this is *probably* safe on systems that have GL_DEBUG_OUTPUT_SYNCHRONOUS enabled. If that
    // turns out to be a fatal assumption then this should be reimplemented to use inter-thread
    // messaging

    std::stringstream ss;
    ss << "OpenGL error detected: id = " << id << ", message = " << message;
    throw std::runtime_error{std::move(ss).str()};
}

static int get_max_multisamples() {
    GLint v = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &v);

    // OpenGL spec: "the value must be at least 4"
    // see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
    OSC_ASSERT(v > 4);

    return v;
}

static bool is_in_opengl_debug_mode() {
    // if context is not debug-mode, then some of the glGet*s below can fail
    // (e.g. GL_DEBUG_OUTPUT_SYNCHRONOUS on apple).
    {
        GLint flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
            return false;
        }
    }

    {
        GLboolean b = false;
        glGetBooleanv(GL_DEBUG_OUTPUT, &b);
        if (!b) {
            return false;
        }
    }

    {
        GLboolean b = false;
        glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &b);
        if (!b) {
            return false;
        }
    }

    return true;
}

static void enable_opengl_debug_mode() {
    if (is_in_opengl_debug_mode()) {
        log::info("application appears to already be in OpenGL debug mode: skipping enabling it");
    }

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glOnDebugMessage, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        log::info("enabled OpenGL debug mode");
    } else {
        log::error("cannot enable OpenGL debug mode: the context does not have GL_CONTEXT_FLAG_DEBUG_BIT set");
    }
}

static void disable_opengl_debug_mode() {
    if (!is_in_opengl_debug_mode()) {
        log::info("application does not need to disable OpenGL debug mode: already in it: skipping");
    }

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glDisable(GL_DEBUG_OUTPUT);
        log::info("disabled OpenGL debug mode");
    } else {
        log::error("cannot disable OpenGL debug mode: the context does not have a GL_CONTEXT_FLAG_DEBUG_BIT set");
    }
}

static int highest_refresh_rate_display() {
    int num_displays = SDL_GetNumVideoDisplays();

    if (num_displays < 1) {
        // this should be impossible but, you know, coding.
        return 60;
    }

    int highest_refresh_rate = 30;
    SDL_DisplayMode mode_struct{};
    for (int display = 0; display < num_displays; ++display) {
        int num_modes = SDL_GetNumDisplayModes(display);
        for (int mode = 0; mode < num_modes; ++mode) {
            SDL_GetDisplayMode(display, mode, &mode_struct);
            highest_refresh_rate = std::max(highest_refresh_rate, mode_struct.refresh_rate);
        }
    }
    return highest_refresh_rate;
}

class Application::Impl final {
public:
    // SDL's application-wide context (inits video subsystem etc.)
    sdl::Context context;

    // SDL active window
    sdl::Window window;

    // SDL OpenGL context
    sdl::GLContext gl;

    // the maximum num multisamples that the OpenGL backend supports
    int max_samples = 1;

    // num multisamples that multisampled renderers should use
    static constexpr int default_samples = 8;
    int samples = 1;

    // ImGui application-wide context
    igx::Context imgui_ctx;

    // ImGui SDL-specific initialization
    igx::SDL2_Context imgui_sdl2_ctx;

    // ImGui OpenGL-specific initialization
    igx::OpenGL3_Context imgui_sdl2_ogl2_ctx;

    // the current screen being drawn by the application
    std::unique_ptr<Screen> current_screen = nullptr;

    // the next screen that the application should show
    //
    // this is typically set when a screen calls `request_transition`
    std::unique_ptr<Screen> requested_screen = nullptr;

    // flag that is set whenever a screen requests that the application should quit
    bool should_quit = false;

    // flag indicating whether the application is in debug mode
    //
    // downstream, this flag is used to determine whether to install OpenGL debug handles, draw
    // debug quads, debug UI elements, etc.
    bool debug_mode = false;

    Impl() :
        // initialize SDL library
        context{SDL_INIT_VIDEO},

        // initialize minimal SDL Window with OpenGL support
        window{[]() {
            log::info("initializing main application (OpenGL 3.3) window");

            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

            // careful about setting resolution, position, etc. - some people have *very* shitty
            // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
            // be <700 px high)
            static constexpr char const* title = "osc";
            static constexpr int x = SDL_WINDOWPOS_CENTERED;
            static constexpr int y = SDL_WINDOWPOS_CENTERED;
            static constexpr int width = 800;
            static constexpr int height = 600;
            static constexpr Uint32 flags =
                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

            return sdl::CreateWindoww(title, x, y, width, height, flags);
        }()},

        // initialize GL context for the application window
        gl{[& window = this->window]() {
            log::info("initializing application OpenGL context");

            sdl::GLContext ctx = sdl::GL_CreateContext(window);

            // enable the context
            if (SDL_GL_MakeCurrent(window, ctx) != 0) {
                throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s + SDL_GetError()};
            }

            // enable vsync by default
            //
            // vsync can feel a little laggy on some systems, but vsync reduces CPU usage
            // on *constrained* systems (e.g. laptops, which the majority of users are using)
            if (SDL_GL_SetSwapInterval(-1) != 0) {
                SDL_GL_SetSwapInterval(1);
            }

            // initialize GLEW
            //
            // effectively, enables the OpenGL API used by this application
            if (auto err = glewInit(); err != GLEW_OK) {
                std::stringstream ss;
                ss << "glewInit() failed: ";
                ss << glewGetErrorString(err);
                throw std::runtime_error{ss.str()};
            }

            // depth testing used to ensure geometry overlaps correctly
            glEnable(GL_DEPTH_TEST);

            // MSXAA is used to smooth out the model
            glEnable(GL_MULTISAMPLE);

            // all vertices in the render are backface-culled
            glEnable(GL_CULL_FACE);

            // print OpenGL information if in debug mode
            log::info(
                "OpenGL initialized: info: %s, %s, (%s), GLSL %s",
                glGetString(GL_VENDOR),
                glGetString(GL_RENDERER),
                glGetString(GL_VERSION),
                glGetString(GL_SHADING_LANGUAGE_VERSION));

            return ctx;
        }()},

        // find out the maximum number of samples the OpenGL backend supports
        max_samples{get_max_multisamples()},

        // set the number of samples multisampled renderers in osc should use
        samples{std::min(max_samples, default_samples)},

        // initialize ImGui
        imgui_ctx{},
        imgui_sdl2_ctx{window, gl},
        imgui_sdl2_ogl2_ctx{OSC_GLSL_VERSION} {
    }

    void internal_start_render_loop(std::unique_ptr<Screen> s) {
        current_screen = std::move(s);

        // main application draw loop (i.e. the "game loop" of this app)
        //
        // implemented an immediate GUI, rather than retained, which is
        // inefficient but makes it easier to add new UI features.
        while (true) {
            // pump events
            for (SDL_Event e; SDL_PollEvent(&e);) {

                // QUIT: quit application
                if (e.type == SDL_QUIT) {
                    return;
                }

                // ImGui: feed event into ImGui
                ImGui_ImplSDL2_ProcessEvent(&e);

                // DEBUG MODE: toggled with F1
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1) {
                    if (is_in_debug_mode()) {
                        disable_debug_mode();
                    } else {
                        enable_debug_mode();
                    }
                }

                // osc::Screen: feed event into the currently-showing osc screen
                current_screen->on_event(e);

                // osc::Screen: handle any possible indirect side-effects the Screen's
                //               `on_event` handler may have had on the application state
                if (should_quit) {
                    return;
                }
                if (requested_screen) {
                    current_screen = std::move(requested_screen);
                    continue;
                }
            }

            // osc::Screen: run `tick`
            current_screen->tick();

            // osc::Screen: handle any possible indirect side-effects the Screen's
            //               `on_event` handler may have had on the application state
            if (should_quit) {
                return;
            }
            if (requested_screen) {
                current_screen = std::move(requested_screen);
                continue;
            }

            // clear the window's framebuffer ready for a new frame to be drawn
            gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // prepare ImGui for a new draw call (an implementation detail of ImGui)
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(
                ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

            // osc::Screen: call current screen's `draw` method
            try {
                current_screen->draw();
            } catch (std::bad_alloc const&) {
                throw;  // don't even try to handle this
            } catch (...) {
                // if drawing the screen threw an exception, then we're potentially
                // kind of fucked, because OpenGL and ImGui might be in an intermediate
                // state (e.g. midway through drawing a window)
                //
                // to *try* and survive, clean up OpenGL and ImGui a little and finalize the
                // draw call *before* throwing, so that the application has a small
                // chance of potentially launching into a different screen (e.g. an
                // error screen)

                gl::UseProgram();

                imgui_sdl2_ogl2_ctx.reset(OSC_GLSL_VERSION);
                imgui_sdl2_ctx.reset(window, gl);
                imgui_ctx.reset();

                SDL_GL_SwapWindow(window);

                throw;
            }

            // edge-case: the screen left its program bound. This can cause issues in the
            //            ImGUI implementation.
            gl::UseProgram();

            // NOTE: osc::Screen side-effects:
            //
            // - The screen's `draw` method *may* have had indirect side-effects on the
            //   application state
            //
            // - However, we finish rendering + swapping the full frame before handling those
            //   side-effects, because ImGui might be in an intermediate state (e.g. it needs
            //   finalizing) and because it might be handy to see the screen *just* before
            //   some kind of transition

            // ImGui: finalize ImGui rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // ImGui: handle multi-viewports if the user has requested them
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
                SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
            }

            // swap the framebuffer frame onto the window, showing it to the user
            //
            // note: this can block on VSYNC, which will affect the timings
            //       for software throttling
            SDL_GL_SwapWindow(window);

            // osc::Screen: handle any possible indirect side-effects the Screen's
            //               `on_event` handler may have had on the application state
            if (should_quit) {
                return;
            }
            if (requested_screen) {
                current_screen = std::move(requested_screen);
                continue;
            }
        }
    }

    void start_render_loop(std::unique_ptr<Screen> s) {
        log::info("starting main render loop");

        bool quit = false;
        while (!quit) {
            try {
                internal_start_render_loop(std::move(s));
                quit = true;
            } catch (std::exception const& ex) {
                // top-level disaster recovery
                //
                // if an exception propagates all the way up here:
                //
                // - print it to the log (handy for Linux/Mac users, and Windows users
                //   can read the log if it's persisted somewhere)
                //
                // - disable debug mode, because it might produce exceptions even while
                //   the error screen is showing, which could cause the error screen to
                //   not render
                //
                // - transition the UI to an error screen that shows the error + log
                //   (handy for general users who don't have a console and don't know
                //   where the log is saved)
                log::error("unhandled exception thrown in main loop: %s", ex.what());
                log::error("transitioning the UI to the error screen");

                disable_debug_mode();

                s = std::make_unique<Error_screen>(ex);
            }
        }
    }

    [[nodiscard]] bool is_in_debug_mode() const noexcept {
        return debug_mode;
    }

    void enable_debug_mode() {
        if (is_in_debug_mode()) {
            return;
        }
        log::info("enabling debug mode");
        debug_mode = true;
        ::enable_opengl_debug_mode();
    }

    void disable_debug_mode() {
        if (!is_in_debug_mode()) {
            return;
        }
        log::info("disabling debug mode");
        debug_mode = false;
        ::disable_opengl_debug_mode();
    }
};

Application::Application() : impl{new Impl{}} {
}

Application::~Application() noexcept = default;

void Application::start_render_loop(std::unique_ptr<Screen> s) {
    impl->start_render_loop(std::move(s));
}

void Application::request_screen_transition(std::unique_ptr<Screen> s) {
    impl->requested_screen = std::move(s);
}

void Application::request_quit_application() {
    impl->should_quit = true;
}

// dimensions of the main application window in pixels
Application::Window_dimensions Application::window_dimensions() const noexcept {
    auto [w, h] = sdl::GetWindowSize(impl->window);
    return {w, h};
}

int Application::samples() const noexcept {
    return impl->samples;
}

int Application::max_samples() const noexcept {
    return impl->max_samples;
}

void Application::set_samples(int s) {
    if (s <= 0) {
        throw std::runtime_error{"tried to set number of samples to <= 0"};
    }

    if (s > max_samples()) {
        throw std::runtime_error{"tried to set number of multisamples higher than supported by hardware"};
    }

    if (num_bits_set_in(s) != 1) {
        throw std::runtime_error{
            "tried to set number of multisamples to an invalid value. Must be 1, or a multiple of 2 (1x, 2x, 4x, 8x...)"};
    }

    impl->samples = s;
}

bool Application::is_in_debug_mode() const noexcept {
    return impl->is_in_debug_mode();
}

void Application::enable_debug_mode() {
    impl->enable_debug_mode();
}

void Application::disable_debug_mode() {
    impl->disable_debug_mode();
}

void Application::make_fullscreen() {
    SDL_SetWindowFullscreen(impl->window, SDL_WINDOW_FULLSCREEN);
}

void Application::make_windowed() {
    SDL_SetWindowFullscreen(impl->window, 0);
}

bool Application::is_vsync_enabled() const noexcept {
    // adaptive vsync (-1) and vsync (1) are treated as "vsync is enabled"
    return SDL_GL_GetSwapInterval() != 0;
}

void Application::enable_vsync() {
    // try using adaptive vsync
    if (SDL_GL_SetSwapInterval(-1) == 0) {
        return;
    }

    // if adaptive vsync doesn't work, then try normal vsync
    if (SDL_GL_SetSwapInterval(1) == 0) {
        return;
    }

    // otherwise, setting vsync isn't supported by the system
}

void Application::disable_vsync() {
    SDL_GL_SetSwapInterval(0);
}
