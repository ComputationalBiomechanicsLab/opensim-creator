#include "add_component_popup.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

#include <memory>
#include <vector>

using namespace osmv;

struct Add_component_popup::Impl final {
    std::unique_ptr<OpenSim::Component> proto;

    // socket assignments
    std::vector<std::string> socket_names = proto->getSocketNames();
    std::vector<OpenSim::PhysicalFrame const*> frame_socket_choices =
        std::vector<OpenSim::PhysicalFrame const*>(socket_names.size());

    Impl(std::unique_ptr<OpenSim::Component> p) : proto{std::move(p)} {
    }
};

Add_component_popup::Add_component_popup(std::unique_ptr<OpenSim::Component> prototype) :
    impl{new Impl{std::move(prototype)}} {
}

Add_component_popup::~Add_component_popup() noexcept = default;

std::unique_ptr<OpenSim::Component> Add_component_popup::draw(char const* modal_name, OpenSim::Model const&) {
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return nullptr;
    }
    // else: draw modal content

    if (ImGui::Button("cancel")) {
        ImGui::CloseCurrentPopup();
    }

    return nullptr;
}
