#include "meshes_to_model_wizard_screen.hpp"

#include <imgui.h>


struct osc::Meshes_to_model_wizard_screen::Impl final {

};


// public API

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen() :
    impl{new Impl} {
}

osc::Meshes_to_model_wizard_screen::~Meshes_to_model_wizard_screen() noexcept = default;

void osc::Meshes_to_model_wizard_screen::draw() {
    ImGui::Text("hello, world!");
}
