#include "application.hpp"

#include "osmv_config.hpp"
#include "gl.hpp"
#include "gl_extensions.hpp"
#include "imgui.h"
#include "screen.hpp"

#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <chrono>


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

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...)
#else
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#endif

osmv::Application::Application() :
    context{sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)},
    window{[]() {
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 2);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_DEPTH_SIZE, 24);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_STENCIL_SIZE, 8);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLEBUFFERS, 1);
       OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLESAMPLES, 16);

       return sdl::CreateWindoww(
           "osmv " OSMV_VERSION_STRING,
           SDL_WINDOWPOS_CENTERED,
           SDL_WINDOWPOS_CENTERED,
           1024,
           768,
           SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    }()},
    gl{sdl::GL_CreateContext(window)},
    imgui_ctx{igx::Context{}},
    imgui_sdl2_ctx{window, gl},
    imgui_sdl2_ogl2_ctx{OSMV_GLSL_VERSION} {

    gl::assert_no_errors("ui::State::constructor::onEnter");
    sdl::GL_SetSwapInterval(0);  // disable VSYNC

    // enable SDL's OpenGL context
    if (SDL_GL_MakeCurrent(window, gl) != 0) {
        throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s  + SDL_GetError()};
    }

    // initialize GLEW, which is what imgui is using under the hood
    if (auto err = glewInit(); err != GLEW_OK) {
        std::stringstream ss;
        ss << "glewInit() failed: ";
        ss << glewGetErrorString(err);
        throw std::runtime_error{ss.str()};
    }

    DEBUG_PRINT("OpenGL info: %s: %s (%s) /w GLSL: %s\n",
                glGetString(GL_VENDOR),
                glGetString(GL_RENDERER),
                glGetString(GL_VERSION),
                glGetString(GL_SHADING_LANGUAGE_VERSION));

    gl::assert_no_errors("ui::State::constructor::onExit");

    glClearColor(0.99f, 0.98f, 0.96f, 1.0f);
    OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);
    OSC_GL_CALL_CHECK(glEnable, GL_BLEND);
    OSC_GL_CALL_CHECK(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);
    OSC_GL_CALL_CHECK(glEnable, GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}



// top-level event pipeline for showing a "screen" to the user
void osmv::Application::show(std::unique_ptr<osmv::Screen> first_screen) {
    current_screen = std::move(first_screen);

    auto last_render_timepoint = std::chrono::high_resolution_clock::now();
    auto min_delay_between_frames = 10ms;

    while (true) {
        // screen: handle pumping events into screen's `handle_event`
        for (SDL_Event e; SDL_PollEvent(&e);) {
            ImGui_ImplSDL2_ProcessEvent(&e);

            if (e.type == SDL_QUIT) {
                return;
            }

            auto resp = current_screen->handle_event(*this, e);
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
            auto resp = current_screen->tick(*this);
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

        // screen: draw
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
        current_screen->draw(*this);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


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
