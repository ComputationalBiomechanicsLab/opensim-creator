#include "imgui_demo_screen.hpp"

#include "src/app.hpp"
#include <imgui.h>

using namespace osc;

void osc::Imgui_demo_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Imgui_demo_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void Imgui_demo_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }
}

void Imgui_demo_screen::draw() {
    osc::ImGuiNewFrame();

    ImGui::ShowDemoWindow();

    osc::ImGuiRender();
}
