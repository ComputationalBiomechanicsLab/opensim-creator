#include "application.hpp"

#include "algs.hpp"
#include "config.hpp"
#include "os.hpp"
#include "osmv_config.hpp"
#include "sdl_wrapper.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/raw_renderer.hpp"
#include "src/screens/error_screen.hpp"
#include "src/screens/screen.hpp"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/imgui.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <optional>
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

// globals
static osmv::Application* g_current_application = nullptr;

struct ImGuiContext;

namespace igx {
    struct Context final {
        std::string ini_dir = (osmv::user_data_dir() / "imgui.ini").string();
        ImGuiContext* handle;

        Context() : handle{ImGui::CreateContext()} {
            configure_context(ImGui::GetIO());
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;

        void configure_context(ImGuiIO& io) {
            io.IniFilename = ini_dir.c_str();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            if (osmv::config::should_use_multi_viewport()) {
                io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }
        }

        void reset() {
            ImGui::DestroyContext(handle);
            handle = ImGui::CreateContext();

            configure_context(ImGui::GetIO());
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

// callback function suitable for glDebugMessageCallback
static void glOnDebugMessage(
    GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei, const char* message, const void*) {

    // ignore non-significant error/warning codes
    // if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    //    return;

    std::cerr << "---------------" << std::endl;
    std::cerr << "Debug message (" << id << "): " << message << std::endl;

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        std::cerr << "Source: API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        std::cerr << "Source: Window System";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        std::cerr << "Source: Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        std::cerr << "Source: Third Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        std::cerr << "Source: Application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        std::cerr << "Source: Other";
        break;
    }
    std::cerr << std::endl;

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        std::cerr << "Type: Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        std::cerr << "Type: Deprecated Behaviour";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        std::cerr << "Type: Undefined Behaviour";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        std::cerr << "Type: Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        std::cerr << "Type: Performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        std::cerr << "Type: Marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        std::cerr << "Type: Push Group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        std::cerr << "Type: Pop Group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        std::cerr << "Type: Other";
        break;
    }
    std::cerr << std::endl;

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        std::cerr << "Severity: high";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        std::cerr << "Severity: medium";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        std::cerr << "Severity: low";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        std::cerr << "Severity: notification";
        break;
    }
    std::cerr << std::endl;

    std::cerr << std::endl;
}

static int get_max_multisamples() {
    GLint v = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &v);

    // OpenGL spec: "the value must be at least 4"
    // see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
    assert(v > 4);
    return v;
}

static void enable_opengl_debug_mode() {
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glOnDebugMessage, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
}

static void disable_opengl_debug_mode() {
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glDisable(GL_DEBUG_OUTPUT);
        glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
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

namespace {
    struct Globally_allocated_mesh_storage final {
        Globally_allocated_mesh_storage() {
            // currently noop
        }
        Globally_allocated_mesh_storage(Globally_allocated_mesh_storage const&) = delete;
        Globally_allocated_mesh_storage(Globally_allocated_mesh_storage&&) = delete;
        Globally_allocated_mesh_storage& operator=(Globally_allocated_mesh_storage const&) = delete;
        Globally_allocated_mesh_storage& operator=(Globally_allocated_mesh_storage&&) = delete;
        ~Globally_allocated_mesh_storage() noexcept {
            osmv::nuke_gpu_allocations();
        }
    };
}

namespace osmv {
    struct Application_impl final {
        // SDL's application-wide context (inits video subsystem etc.)
        sdl::Context context;

        // SDL active window
        sdl::Window window;

        // SDL OpenGL context
        sdl::GLContext gl;

        // Setup global mesh storage
        Globally_allocated_mesh_storage global_mesh_storage;

        // the maximum num multisamples that the OpenGL backend supports
        int max_samples = 1;

        // num multisamples that multisampled renderers should use
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

        // flag indicating whether the UI should draw certain debug UI elements (e.g. FPS counter,
        // debug overlays)
        bool is_drawing_debug_ui = false;

        Application_impl() :
            // initialize SDL library
            context{SDL_INIT_VIDEO},

            // initialize minimal SDL Window with OpenGL 3.2 support
            window{[]() {
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);

                // careful about setting resolution, position, etc. - some people have *very* shitty
                // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
                // be <700 px high)
                static constexpr char const* title = "osmv";
                static constexpr int x = SDL_WINDOWPOS_CENTERED;
                static constexpr int y = SDL_WINDOWPOS_CENTERED;
                static constexpr int width = 800;
                static constexpr int height = 600;
                static constexpr Uint32 flags =
                    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

                return sdl::CreateWindoww(title, x, y, width, height, flags);
            }()},

            // initialize GL context for the application window
            gl{[this]() {
                sdl::GLContext ctx = sdl::GL_CreateContext(window);

                // enable the context
                if (SDL_GL_MakeCurrent(window, gl) != 0) {
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
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

                // MSXAA is used to smooth out the model
                glEnable(GL_MULTISAMPLE);
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

                // all vertices in the render are backface-culled
                glEnable(GL_CULL_FACE);
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

                return ctx;
            }()},

            // find out the maximum number of samples the OpenGL backend supports
            max_samples{get_max_multisamples()},

            // set the number of samples multisampled renderers in osmv should use
            samples{std::min(max_samples, 8)},

            // initialize ImGui
            imgui_ctx{},
            imgui_sdl2_ctx{window, gl},
            imgui_sdl2_ogl2_ctx{OSMV_GLSL_VERSION} {

            // any other initialization fixups
#ifndef NDEBUG
            enable_opengl_debug_mode();
            std::cerr << "OpenGL: " << glGetString(GL_VENDOR) << ", " << glGetString(GL_RENDERER) << "("
                      << glGetString(GL_VERSION) << "), GLSL " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
#endif
        }

        void internal_start_render_loop(Application& app, std::unique_ptr<Screen> s) {
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
                    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_F1) {
                        is_drawing_debug_ui = not is_drawing_debug_ui;
                    }

                    // OpenGL DEBUG MODE: enabled (not toggled) with F2
                    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_F2) {
                        std::cerr << "enabling OpenGL debug mode (GL_DEBUG_OUTPUT)" << std::endl;
                        ::enable_opengl_debug_mode();
                    }

                    // osmv::Screen: feed event into the currently-showing osmv screen
                    current_screen->on_event(e);

                    // osmv::Screen: handle any possible indirect side-effects the Screen's
                    //               `on_event` handler may have had on the application state
                    if (should_quit) {
                        return;
                    }
                    if (requested_screen) {
                        current_screen = std::move(requested_screen);
                        continue;
                    }
                }

#ifndef NDEBUG
                // debug OpenGL: assert no OpenGL errors were induced by event handling
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

                // osmv::Screen: run `tick`
                current_screen->tick();

                // osmv::Screen: handle any possible indirect side-effects the Screen's
                //               `on_event` handler may have had on the application state
                if (should_quit) {
                    return;
                }
                if (requested_screen) {
                    current_screen = std::move(requested_screen);
                    continue;
                }

#ifndef NDEBUG
                // debug OpenGL: assert no OpenGL errors were induced by .tick()
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

                // clear the window's framebuffer ready for a new frame to be drawn
                gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // prepare ImGui for a new draw call (an implementation detail of ImGui)
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame(window);
                ImGui::NewFrame();

                ImGui::DockSpaceOverViewport(
                    ImGui::GetMainViewport(),
                    ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

                // osmv::Screen: call current screen's `draw` method
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

                    imgui_sdl2_ogl2_ctx.reset(OSMV_GLSL_VERSION);
                    imgui_sdl2_ctx.reset(window, gl);
                    imgui_ctx.reset();

                    SDL_GL_SwapWindow(window);

                    throw;
                }

                // edge-case: the screen left its program bound. This can cause issues in the
                //            ImGUI implementation.
                gl::UseProgram();

#ifndef NDEBUG
                // debug OpenGL: assert no OpenGL errors were induced by .draw()
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

                // NOTE: osmv::Screen side-effects:
                //
                // - The screen's `draw` method *may* have had indirect side-effects on the
                //   application state
                //
                // - However, we finish rendering + swapping the full frame before handling those
                //   side-effects, because ImGui might be in an intermediate state (e.g. it needs
                //   finalizing) and because it might be handy to see the screen *just* before
                //   some kind of transition

                // draw FPS overlay in bottom-right: handy for dev
                if (is_drawing_debug_ui) {
                    char buf[16];
                    double fps = static_cast<double>(ImGui::GetIO().Framerate);
                    std::snprintf(buf, sizeof(buf), "%.0f", fps);
                    sdl::Window_dimensions d = sdl::GetWindowSize(window);
                    ImVec2 window_sims = {static_cast<float>(d.w), static_cast<float>(d.h)};
                    ImVec2 font_dims = ImGui::CalcTextSize(buf);
                    ImVec2 fps_pos = {window_sims.x - font_dims.x, window_sims.y - font_dims.y};
                    ImGui::GetBackgroundDrawList()->AddText(fps_pos, 0xff0000ff, buf);
                }

                // ImGui: finalize ImGui rendering
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

#ifndef NDEBUG
                // debug OpenGL: assert no OpenGL errors induced by final draw steps
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

                // osmv::Screen: handle any possible indirect side-effects the Screen's
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

        void start_render_loop(Application& app, std::unique_ptr<Screen> s) {
            bool quit = false;
            while (not quit) {
                try {
                    internal_start_render_loop(app, std::move(s));
                    quit = true;
                } catch (std::exception const& ex) {
                    // if an exception is thrown all the way up here, print it
                    // to the stdout/stderr (Linux/Mac users with decent consoles)
                    // but also throw up a basic error message GUI (Windows users)
                    s = std::make_unique<Error_screen>(ex);
                }
            }
        }

        void request_transition(std::unique_ptr<osmv::Screen> s) {
            requested_screen = std::move(s);
        }

        void request_quit() {
            should_quit = true;
        }
    };
}

osmv::Application::Application() : impl{new Application_impl{}} {
}

osmv::Application::~Application() noexcept = default;

void osmv::Application::start_render_loop(std::unique_ptr<Screen> s) {
    impl->start_render_loop(*this, std::move(s));
}

void osmv::Application::request_screen_transition(std::unique_ptr<osmv::Screen> s) {
    impl->request_transition(std::move(s));
}

void osmv::Application::request_quit_application() {
    impl->request_quit();
}

// dimensions of the main application window in pixels
osmv::Window_dimensions osmv::Application::window_dimensions() const noexcept {
    auto [w, h] = sdl::GetWindowSize(impl->window);
    return Window_dimensions{w, h};
}

// move mouse relative to the window (origin in top-left)
void osmv::Application::move_mouse_to(int x, int y) {
    SDL_WarpMouseInWindow(impl->window, x, y);
}

int osmv::Application::samples() const noexcept {
    return impl->samples;
}

int osmv::Application::max_samples() const noexcept {
    return impl->max_samples;
}

void osmv::Application::set_samples(int s) {
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

    // push a SamplesChanged event into the event queue so that downstream screens can change any
    // internal renderers/buffers to match
    SDL_Event e;
    e.type = SDL_USEREVENT;
    e.user.code = OsmvCustomEvent_SamplesChanged;
    SDL_PushEvent(&e);
}

bool osmv::Application::is_in_debug_mode() const noexcept {
    return impl->is_drawing_debug_ui;
}

void osmv::Application::make_fullscreen() {
    SDL_SetWindowFullscreen(impl->window, SDL_WINDOW_FULLSCREEN);
}

void osmv::Application::make_windowed() {
    SDL_SetWindowFullscreen(impl->window, 0);
}

bool osmv::Application::is_vsync_enabled() const noexcept {
    // adaptive vsync (-1) and vsync (1) are treated as "vsync is enabled"
    return SDL_GL_GetSwapInterval() != 0;
}

void osmv::Application::enable_vsync() {
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

void osmv::Application::disable_vsync() {
    SDL_GL_SetSwapInterval(0);
}

void osmv::set_current_application(Application* app) {
    g_current_application = app;
}

osmv::Application& osmv::app() noexcept {
    assert(g_current_application != nullptr);
    return *g_current_application;
}
