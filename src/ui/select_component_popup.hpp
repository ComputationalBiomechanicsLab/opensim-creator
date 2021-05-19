#include <OpenSim/Common/Component.h>
#include <imgui.h>

// popup for selecting a component of the specified type
namespace osc::ui::select_component {
    struct State final {};

    // returns non-nullptr if user selects a component (that derives from) the specified type
    //
    // expects the caller to handle ImGui::OpenPopup
    template<typename T>
    T const* draw(State& st, char const* modal_name, OpenSim::Component const& root) {

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

        T const* selected = nullptr;

        ImGui::BeginChild("first", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (T const& c : root.getComponentList<T>()) {
            if (ImGui::Button(c.getName().c_str())) {
                selected = &c;
            }
        }
        ImGui::EndChild();

        if (selected || ImGui::Button("cancel")) {
            st = {};  // reset user inputs
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();

        return selected;
    }
}