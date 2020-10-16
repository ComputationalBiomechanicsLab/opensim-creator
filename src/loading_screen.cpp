#include "loading_screen.hpp"

#include "imgui.h"
#include "imgui_extensions.hpp"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"
#include "gl.hpp"
#include "application.hpp"
#include "show_model_screen.hpp"
#include <iostream>
#include <chrono>
#include "sdl.hpp"

using std::chrono_literals::operator""ms;

osmv::Loading_screen::Loading_screen(std::string _path) :
    path{std::move(_path)},
    result{std::async(std::launch::async, [&]() {
        auto geom = osim::geometry_in(path.c_str());
        // push an event so that the UI's event handler bubbles an event through
        // the stack
        SDL_Event e;
        e.type = SDL_USEREVENT;
        SDL_PushEvent(&e);
        return geom;
    })} {
}

void osmv::Loading_screen::init(Application&) {
    // load the model on a background thread that is polled for completion

}

osmv::Screen_response osmv::Loading_screen::handle_event(Application&, SDL_Event&) {
    if (result.wait_for(0ms) == std::future_status::ready) {
        return Resp_Transition_to{std::make_unique<Show_model_screen>(result.get())};
    } else {
        return Resp_Ok{};
    }
}

void osmv::Loading_screen::draw(Application& s) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(s.window);

    ImGui::NewFrame();

    {
        bool b = true;
        ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar);
    }

    ImGui::Text((std::string("loading: ") + path).c_str());

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
