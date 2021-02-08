#include "application.hpp"

#include "gl.hpp"
#include "osmv_config.hpp"
#include "screen.hpp"
#include "sdl_wrapper.hpp"

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

#define OSC_GL_CALL_CHECK(func, ...)                                                                                   \
    {                                                                                                                  \
        func(__VA_ARGS__);                                                                                             \
        gl::assert_no_errors(#func);                                                                                   \
    }

using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;

// globals
std::unique_ptr<osmv::Application> osmv::_current_app;

struct ImGuiContext;

namespace igx {
    class Context final {
        ImGuiContext* handle;

    public:
        Context() : handle{ImGui::CreateContext()} {
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
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
        ~OpenGL3_Context() noexcept {
            ImGui_ImplOpenGL3_Shutdown();
        }
    };
}

// callback function suitable for glDebugMessageCallback
static void glOnDebugMessage(
    GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei, const char* message, const void*) {

    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

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

static GLsizei get_max_multisamples() {
    GLint v = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &v);
    v = std::min(v, 8);
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

namespace osmv {
    struct Application_impl final {
        // SDL's application-wide context (inits video subsystem etc.)
        sdl::Context context;

        // SDL active window
        sdl::Window window;

        // SDL OpenGL context
        sdl::GLContext gl;

        // *highest* refresh rate display on user's machine
        int refresh_rate = 60;

        // milliseconds between frames - for software throttling
        std::chrono::milliseconds millis_between_frames{static_cast<int>(1000.0 * (1.0 / refresh_rate))};

        // num multisamples that multisampled renderers should use
        int samples = 1;

        // ImGui application-wide context
        igx::Context imgui_ctx;

        // ImGui SDL-specific initialization
        igx::SDL2_Context imgui_sdl2_ctx;

        // ImGui OpenGL-specific initialization
        igx::OpenGL3_Context imgui_sdl2_ogl2_ctx;

        // whether the application should sleep the CPU when the FPS exceeds some
        // amount (ideally, throttling should keep the UI refresh rate close to the
        // screen refresh rate)
        bool software_throttle = true;

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
            context{SDL_INIT_VIDEO | SDL_INIT_TIMER},

            // initialize minimal SDL Window with OpenGL 3.2 support
            window{[]() {
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);

                static constexpr char const* title = "osmv";
                static constexpr int x = SDL_WINDOWPOS_CENTERED;
                static constexpr int y = SDL_WINDOWPOS_CENTERED;
                static constexpr int width = 1024;
                static constexpr int height = 768;
                static constexpr Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

                return sdl::CreateWindoww(title, x, y, width, height, flags);
            }()},

            // initialize GL context for the application window
            gl{[this]() {
                sdl::GLContext ctx = sdl::GL_CreateContext(window);

                // enable the context
                if (SDL_GL_MakeCurrent(window, gl) != 0) {
                    throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s + SDL_GetError()};
                }

                // disable VSync
                //
                // UI uses software throttling because vsync can be laggy on some platforms
                sdl::GL_SetSwapInterval(0);

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
                OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);

                // MSXAA is used to smooth out the model
                OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);

                // all vertices in the render are backface-culled
                OSC_GL_CALL_CHECK(glEnable, GL_CULL_FACE);
                glFrontFace(GL_CCW);

                // ensure none of the above triggered a global OpenGL error
                gl::assert_no_errors("Application::constructor");

                return ctx;
            }()},

            // initialize refresh rate as the highest refresh-rate display mode on the computer
            refresh_rate{[]() {
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
            }()},

            // millis between frames (for throttling) is based on the highest refresh rate
            millis_between_frames{static_cast<int>(1000.0 * (1.0 / refresh_rate))},

            // just set multisamples to max for now
            samples{get_max_multisamples()},

            // initialize ImGui
            imgui_ctx{},
            imgui_sdl2_ctx{window, gl},
            imgui_sdl2_ogl2_ctx{OSMV_GLSL_VERSION} {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            // any other initialization fixups
#ifndef NDEBUG
            enable_opengl_debug_mode();
            std::cerr << "OpenGL: " << glGetString(GL_VENDOR) << ", " << glGetString(GL_RENDERER) << "("
                      << glGetString(GL_VERSION) << "), GLSL " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
#endif
        }

        void start_render_loop(Application& app, std::unique_ptr<Screen> s) {
            current_screen = std::move(s);

            // main application draw loop (i.e. the "game loop" of this app)
            //
            // implemented an immediate GUI, rather than retained, which is
            // inefficient but makes it easier to add new UI features.
            while (true) {
                auto frame_start = std::chrono::high_resolution_clock::now();

                // pump events
                for (SDL_Event e; SDL_PollEvent(&e);) {

                    // QUIT: quit application
                    if (e.type == SDL_QUIT) {
                        return;
                    }

                    // WINDOW EVENT: if it's a resize event, adjust the OpenGL viewport
                    //               to reflect the new dimensions
                    if (e.type == SDL_WINDOWEVENT and e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        int w = e.window.data1;
                        int h = e.window.data2;
                        glViewport(0, 0, w, h);
                    }

                    // DEBUG MODE: toggled with F1
                    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_F1) {
                        is_drawing_debug_ui = not is_drawing_debug_ui;
                    }

                    // ImGui: feed event into ImGui
                    {
                        ImGui_ImplSDL2_ProcessEvent(&e);
                        ImGuiIO& io = ImGui::GetIO();

                        // if ImGui wants mouse/keyboard then we're done with this event, see
                        // comments in ImGui_ImplSDL2_ProcessEvent
                        if (e.type == SDL_KEYDOWN) {
                            if (io.WantTextInput) {
                                continue;
                            }
                        } else if (io.WantCaptureMouse) {
                            continue;
                        }
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

                // clear the window's framebuffer ready for a new frame to be drawn
                gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // prepare ImGui for a new draw call (an implementation detail of ImGui)
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame(window);
                ImGui::NewFrame();

                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

                // osmv::Screen: call current screen's `draw` method
                current_screen->draw();

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

                // swap the framebuffer frame onto the window, showing it to the user
                //
                // note: this can block on VSYNC, which will affect the timings
                //       for software throttling
                SDL_GL_SwapWindow(window);

                // osmv::Screen: handle any possible indirect side-effects the Screen's
                //               `on_event` handler may have had on the application state
                if (should_quit) {
                    return;
                }
                if (requested_screen) {
                    current_screen = std::move(requested_screen);
                    continue;
                }

                // throttle the framerate, if requested
                if (software_throttle) {
                    // APPROXIMATION: rendering **the next frame** will take roughly as long as it
                    // took to render this frame. Assume worst case is 3x longer (also, the thread
                    // might wake up a little late).

                    auto frame_end = std::chrono::high_resolution_clock::now();
                    auto this_frame_dur = frame_end - frame_start;
                    auto next_frame_estimation = 4 * this_frame_dur;
                    if (next_frame_estimation < millis_between_frames) {
                        auto dt = millis_between_frames - next_frame_estimation;
                        auto dt_millis = std::chrono::duration_cast<std::chrono::milliseconds>(dt);
                        SDL_Delay(static_cast<Uint32>(dt_millis.count()));
                    }
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

bool osmv::Application::is_throttling_fps() const noexcept {
    return impl->software_throttle;
}

void osmv::Application::is_throttling_fps(bool throttle) {
    impl->software_throttle = throttle;
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
    return SDL_GL_GetSwapInterval() == 1;
}

void osmv::Application::enable_vsync() {
    SDL_GL_SetSwapInterval(1);
}

void osmv::Application::disable_vsync() {
    SDL_GL_SetSwapInterval(0);
}

void osmv::Application::enable_opengl_debug_mode() {
    ::enable_opengl_debug_mode();
}

void osmv::Application::disable_opengl_debug_mode() {
    ::disable_opengl_debug_mode();
}

void osmv::init_application() {
    _current_app = std::make_unique<Application>();
}
