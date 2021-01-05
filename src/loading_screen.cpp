#include "loading_screen.hpp"

#include "show_model_screen.hpp"

#include "gl.hpp"
#include "imgui.h"

#include <chrono>


using std::chrono_literals::operator""ms;

osmv::Loading_screen::Loading_screen(std::string _path) :
    path{std::move(_path)},
    result{std::async(std::launch::async, [&]() {
        return osmv::load_osim(path.c_str());
    })} {
}

osmv::Screen_response osmv::Loading_screen::tick(Application&) {
    if (result.wait_for(0ms) == std::future_status::ready) {
        return Resp_transition{std::make_unique<Show_model_screen>(path, result.get())};
    } else {
        return Resp_ok{};
    }
}

void osmv::Loading_screen::draw(Application&) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool b = true;
    if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar)) {
        ImGui::Text("loading: %s", path.c_str());
        ImGui::End();
    }
}
