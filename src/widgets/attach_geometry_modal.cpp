#include "attach_geometry_modal.hpp"

#include "src/utils/indirect_ptr.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <imgui.h>

#include <cstddef>
#include <memory>
#include <string>

void osmv::Attach_geometry_modal::show() {
    ImGui::OpenPopup(name);
}

void osmv::Attach_geometry_modal::draw(
    std::vector<std::filesystem::path> const& vtps, Indirect_ptr<OpenSim::PhysicalFrame>& frame) {

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (not ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::InputText("search", search, sizeof(search));

    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (not recent.empty()) {
        // recent
        ImGui::Text("recent:");
        ImGui::BeginChild(
            "recent meshes", ImVec2(ImGui::GetContentRegionAvail().x, 64), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (std::filesystem::path const& p : recent) {
            if (p.filename().compare(search)) {
                if (ImGui::Selectable(p.filename().string().c_str())) {
                    auto guard = frame.modify();

                    guard->updProperty_attached_geometry().clear();
                    // TODO: this should check whether the mesh is findable via its filename
                    guard->attachGeometry(new OpenSim::Mesh{p.filename().string()});
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndChild();
        ImGui::Dummy(ImVec2{0.0f, 1.0f});
    }

    // all
    ImGui::Text("all:");
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("all meshes", ImVec2(ImGui::GetContentRegionAvail().x, 256), false, window_flags);
    for (size_t i = 0; i < vtps.size(); ++i) {
        std::string filename = vtps[i].filename().string();

        if (filename.find(search) != std::string::npos) {
            if (ImGui::Selectable(filename.c_str())) {
                std::filesystem::path const& vtp = vtps[i];

                auto guard = frame.modify();
                guard->updProperty_attached_geometry().clear();
                // TODO: this should check whether the mesh is findable via its filename
                guard->attachGeometry(new OpenSim::Mesh{vtp.filename().string()});
                recent.push_back(vtp);
                ImGui::CloseCurrentPopup();
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
