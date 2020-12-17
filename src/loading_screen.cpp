#include "loading_screen.hpp"

#include "imgui.h"
#include "imgui_extensions.hpp"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"
#include "gl.hpp"
#include "application.hpp"
#include "globals.hpp"
#include "show_model_screen.hpp"
#include <iostream>
#include <chrono>
#include "sdl.hpp"

using std::chrono_literals::operator""ms;

osmv::Loading_screen::Loading_screen(std::string _path) :
    path{std::move(_path)},
    result{std::async(std::launch::async, [&]() {
        return osim::load_osim(path.c_str());
    })} {

    osmv::log_perf_bootup_event("initialized loading screen");
}

osmv::Screen_response osmv::Loading_screen::tick(Application&) {
    if (result.wait_for(0ms) == std::future_status::ready) {
        osmv::log_perf_bootup_event("loaded model");
        return Resp_Transition_to{std::make_unique<Show_model_screen>(path, result.get())};
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
