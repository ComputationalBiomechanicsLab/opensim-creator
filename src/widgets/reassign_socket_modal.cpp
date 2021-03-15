#include "reassign_socket_modal.hpp"

#include "src/utils/indirect_ref.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

void osmv::Reassign_socket_modal::show(char const* modal_name) {
    ImGui::OpenPopup(modal_name);
}

void osmv::Reassign_socket_modal::close() {
    last_connection_error.clear();
    search[0] = '\0';
    ImGui::CloseCurrentPopup();
}

void osmv::Reassign_socket_modal::draw(
    char const* modal_name, Indirect_ref<OpenSim::Model>& model, Indirect_ref<OpenSim::AbstractSocket>& socket) {

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (not ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::InputText("search", search, sizeof(search));

    ImGui::Text("objects:");
    ImGui::BeginChild("obj list", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (OpenSim::Component const& c : model.get().getComponentList()) {
        std::string name = c.getName();
        if (name.find(search) != std::string::npos) {
            if (ImGui::Selectable(c.getName().c_str())) {
                OpenSim::Object const& existing = socket.get().getConnecteeAsObject();
                try {
                    socket.apply_modification([&c](OpenSim::AbstractSocket& s) { s.connect(c); });
                    model.apply_modification([](OpenSim::Model& m) { m.finalizeConnections(); });

                    this->close();
                } catch (std::exception const& ex) {
                    last_connection_error = ex.what();

                    socket.apply_modification([&existing](OpenSim::AbstractSocket& s) { s.connect(existing); });
                }
            }
        }
    }
    ImGui::EndChild();

    if (not last_connection_error.empty()) {
        ImGui::Text("%s", last_connection_error.c_str());
    }

    if (ImGui::Button("Cancel")) {
        this->close();
    }

    ImGui::EndPopup();
}
