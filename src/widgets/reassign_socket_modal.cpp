#include "reassign_socket_modal.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

void osmv::draw_reassign_socket_modal(
    Reassign_socket_modal_state& st,
    char const* modal_name,
    OpenSim::Model const& model,
    OpenSim::AbstractSocket const& socket,
    std::function<void(OpenSim::Object const&)> const& on_conectee_change_request) {

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (not ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::InputText("search", st.search, sizeof(st.search));

    ImGui::Text("objects:");
    ImGui::BeginChild("obj list", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (OpenSim::Component const& c : model.getComponentList()) {
        std::string name = c.getName();
        if (name.find(st.search) != std::string::npos) {
            if (ImGui::Selectable(c.getName().c_str())) {
                OpenSim::Object const& existing = socket.getConnecteeAsObject();
                try {
                    on_conectee_change_request(c);
                    st.search[0] = '\0';
                    ImGui::CloseCurrentPopup();
                } catch (std::exception const& ex) {
                    st.error = ex.what();
                    on_conectee_change_request(existing);
                }
            }
        }
    }
    ImGui::EndChild();

    if (not st.error.empty()) {
        ImGui::Text("%s", st.error.c_str());
    }

    if (ImGui::Button("Cancel")) {
        st.search[0] = '\0';
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
