#include "application.hpp"

#include "osmv_config.hpp"
#include "gl.hpp"
#include "gl_extensions.hpp"
#include "imgui.h"
#include "screen.hpp"
#include "imgui_extensions.hpp"

#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <chrono>
#include <cassert>


using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#define OSC_SDL_GL_SetAttribute_CHECK(attr, value) { \
        int rv = SDL_GL_SetAttribute((attr), (value)); \
        if (rv != 0) { \
            throw std::runtime_error{"SDL_GL_SetAttribute failed when setting " #attr " = " #value " : "s + SDL_GetError()}; \
        } \
    }

// macros for quality-of-life checks
#define OSC_GL_CALL_CHECK(func, ...) { \
        func(__VA_ARGS__); \
        gl::assert_no_errors(#func); \
    }

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...)
#else
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#endif

// top-level screenbuffer: potentially-multisampled framebuffer that is separate from
// the window's framebuffer
//
// the screen's FBO is separate from the window's because there are post-window-initialization
// checks that need to be performed (e.g. does the user's computer support MSXAA?) and because
// architecting it this way enables the user to change top-level graphics settings inside the
// application window without having to reboot the entire application
//
// it also enables screen-wide post-processing (e.g. blurring), but that isn't exercised here.

class Screen_framebuffer final {
    sdl::Window_dimensions dims;
    gl::Render_buffer color0_rbo = gl::GenRenderBuffer();
    gl::Render_buffer depth24stencil8_rbo = gl::GenRenderBuffer();
    gl::Frame_buffer fbo = gl::GenFrameBuffer();

public:
    Screen_framebuffer(sdl::Window_dimensions const& _dims, GLsizei samples) : dims{_dims} {
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);

        // bind, allocate, and link color0's RBO to the FBO
        gl::BindRenderBuffer(color0_rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA, dims.w, dims.h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color0_rbo.handle);

        // bind, allocate, and link depth+stencil RBOs to the FBO
        gl::BindRenderBuffer(depth24stencil8_rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, dims.w, dims.h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth24stencil8_rbo.handle);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // reset FBO back to default window FBO
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    operator gl::Frame_buffer const& () const noexcept {
        return fbo;
    }

    operator gl::Frame_buffer& () noexcept {
        return fbo;
    }

    [[nodiscard]] sdl::Window_dimensions dimensions() const noexcept {
        return dims;
    }

    [[nodiscard]] int width() const noexcept {
        return dims.w;
    }

    [[nodiscard]] int height() const noexcept {
        return dims.h;
    }
};

static GLsizei get_max_multisamples() {
    GLint v = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &v);
    v = std::min(v, 16);
    return v;
}

namespace osmv {
    struct Application_impl final {
        // SDL's application-wide context (inits video subsystem etc.)
        sdl::Context context;

        // SDL active window
        sdl::Window window;

        // SDL OpenGL context
        sdl::GLContext gl;

        // the (potentially multisampled) framebuffer that screens draws into
        Screen_framebuffer sfbo;

        // ImGui application-wide context
        igx::Context imgui_ctx;

        // ImGui SDL-specific initialization
        igx::SDL2_Context imgui_sdl2_ctx;

        // ImGui OpenGL-specific initialization
        igx::OpenGL3_Context imgui_sdl2_ogl2_ctx;

        // whether the application should sleep the CPU when the FPS exceeds some
        // amount (ideally, close to the screen refresh rate)
        bool software_throttle = true;

        // the current screen being drawn by the application
        std::unique_ptr<Screen> current_screen;

        Application_impl() :
            // initialize SDL library
            context{sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)},

            // initialize minimal SDL Window with OpenGL 3.2 support
            window{[]() {
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 2);

                static constexpr char const* title = "osmv " OSMV_VERSION_STRING;
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
                    throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s  + SDL_GetError()};
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

                // print OpenGL driver info
                DEBUG_PRINT("OpenGL info: %s: %s (%s) /w GLSL: %s\n",
                            glGetString(GL_VENDOR),
                            glGetString(GL_RENDERER),
                            glGetString(GL_VERSION),
                            glGetString(GL_SHADING_LANGUAGE_VERSION));

                // initialize any top-level OpenGL vars
                glClearColor(0.99f, 0.98f, 0.96f, 1.0f);
                OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);
                OSC_GL_CALL_CHECK(glEnable, GL_BLEND);
                OSC_GL_CALL_CHECK(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);
                OSC_GL_CALL_CHECK(glEnable, GL_CULL_FACE);
                glFrontFace(GL_CCW);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                // ensure none of the above triggered a global OpenGL error
                gl::assert_no_errors("Application::constructor");

                return ctx;
            }()},

            // initialize the non-window FBO that the application writes into
            sfbo{sdl::GetWindowSize(window), get_max_multisamples()},

            // initialize ImGui
            imgui_ctx{igx::Context{}},
            imgui_sdl2_ctx{window, gl},
            imgui_sdl2_ogl2_ctx{OSMV_GLSL_VERSION} {
        }

        void show(Application& app, std::unique_ptr<osmv::Screen> first_screen) {
            current_screen = std::move(first_screen);

            auto last_render_timepoint = std::chrono::high_resolution_clock::now();
            auto min_delay_between_frames = 10ms;

            while (true) {
                // screen: handle pumping events into screen's `handle_event`
                for (SDL_Event e; SDL_PollEvent(&e);) {

                    // edge-case: the event is a screen resize, which means that the
                    // screen buffers must be resized
                    if (e.type == SDL_WINDOWEVENT and
                        e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

                        sdl::Window_dimensions new_dims{e.window.data1, e.window.data2};
                        if (new_dims != sfbo.dimensions()) {
                            sfbo = Screen_framebuffer{new_dims, get_max_multisamples()};
                        }
                    }

                    ImGui_ImplSDL2_ProcessEvent(&e);

                    if (e.type == SDL_QUIT) {
                        return;
                    }

                    auto resp = current_screen->handle_event(app, e);
                    bool should_quit = false;
                    bool just_changed_screen = false;
                    std::unique_ptr<Screen> new_screen;
                    std::visit(overloaded {
                        [&](Resp_quit const&) {
                            should_quit = true;
                        },
                        [&](Resp_transition& tgt) {
                            current_screen = std::move(tgt.new_screen);
                            just_changed_screen = true;
                        },
                        [&](Resp_ok const&) {
                        }}, resp);

                    if (should_quit) {
                        return;
                    }

                    if (just_changed_screen) {
                        continue;
                    }
                }

                // screen: handle `tick`
                {
                    auto resp = current_screen->tick(app);
                    bool should_quit = false;
                    bool just_changed_screen = false;
                    std::unique_ptr<Screen> new_screen;
                    std::visit(overloaded {
                        [&](Resp_quit const&) {
                            should_quit = true;
                        },
                        [&](Resp_transition& tgt) {
                            current_screen = std::move(tgt.new_screen);
                            just_changed_screen = true;
                        },
                        [&](Resp_ok const&) {
                        }}, resp);

                    if (should_quit) {
                        return;
                    }

                    if (just_changed_screen) {
                        continue;
                    }
                }

                // draw call:
                //
                // - application draws into a separate screen buffer
                // - that buffer is blitted to the window's buffer

                // bind the screen buffer
                gl::BindFrameBuffer(GL_FRAMEBUFFER, sfbo);

                // draw into the screen buffer
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame(window);
                ImGui::NewFrame();
                current_screen->draw(app);
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                // screen buffer now populated: blit it to the window
                gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, sfbo);
                gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, gl::window_fbo);
                assert(sfbo.dimensions() == sdl::GetWindowSize(window));
                gl::BlitFramebuffer(0, 0, sfbo.width(), sfbo.height(), 0, 0, sfbo.width(), sfbo.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);


                // non-screen-specific stuff

                if (software_throttle) {
                    // software-throttle the framerate: no need to draw at an insane
                    // (e.g. 2000 FPS, on my machine) FPS, but do not use VSYNC because
                    // it makes the entire application feel *very* laggy.
                    auto now = std::chrono::high_resolution_clock::now();
                    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_timepoint);
                    if (dt < min_delay_between_frames) {
                        SDL_Delay(static_cast<Uint32>((min_delay_between_frames - dt).count()));
                    }
                }

                // swap the draw buffers
                SDL_GL_SwapWindow(window);
                last_render_timepoint = std::chrono::high_resolution_clock::now();
            }
        }
    };
}

osmv::Application::Application() : impl{new Application_impl{}} {
}
osmv::Application::~Application() noexcept = default;

void osmv::Application::show(std::unique_ptr<osmv::Screen> s) {
   impl->show(*this, std::move(s));
}

bool osmv::Application::is_fps_throttling() const noexcept {
    return impl->software_throttle;
}

void osmv::Application::enable_fps_throttling() noexcept {
    impl->software_throttle = true;
}

void osmv::Application::disable_fps_throttling() noexcept {
    impl->software_throttle = false;
}

// dimensions of the main application window in pixels
sdl::Window_dimensions osmv::Application::window_size() const noexcept {
    return sdl::GetWindowSize(impl->window);
}

float osmv::Application::aspect_ratio() const noexcept {
    auto [w, h] = window_size();
    return static_cast<float>(w) / static_cast<float>(h);
}

// move mouse relative to the window (origin in top-left)
void osmv::Application::move_mouse_to(int x, int y) {
    SDL_WarpMouseInWindow(impl->window, x, y);
}
