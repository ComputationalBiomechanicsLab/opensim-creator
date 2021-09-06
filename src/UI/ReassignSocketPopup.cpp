#include "ReassignSocketPopup.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

OpenSim::Object const* osc::ReassignSocketPopup::draw(
    char const* popupName,
    OpenSim::Model const& model,
    OpenSim::AbstractSocket const&) {

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return nullptr;
    }

    ImGui::InputText("search", search, sizeof(search));

    ImGui::TextUnformatted("objects:");
    ImGui::BeginChild("obj list", ImVec2(512, 256), true, ImGuiWindowFlags_HorizontalScrollbar);

    OpenSim::Object const* rv = nullptr;

    for (OpenSim::Component const& c : model.getComponentList()) {
        std::string const& name = c.getName();
        if (name.find(search) != std::string::npos) {
            if (ImGui::Selectable(name.c_str())) {
                if (rv == nullptr) {
                    rv = &c;
                }
            }
        }
    }
    ImGui::EndChild();

    if (!error.empty()) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        ImGui::TextWrapped("%s", error.c_str());
    }

    if (ImGui::Button("Cancel")) {
        error.clear();
        search[0] = '\0';
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}
