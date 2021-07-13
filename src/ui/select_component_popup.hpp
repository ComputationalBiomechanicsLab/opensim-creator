#include <OpenSim/Common/Component.h>
#include <imgui.h>

// popup for selecting a component of the specified type
namespace osc::ui::select_component {

    // returns non-nullptr if user selects a component (that derives from) the specified type
    //
    // expects the caller to handle ImGui::OpenPopup
    template<typename T>
    T const* draw(char const* modal_name, OpenSim::Component const& root) {

        // center the modal
        {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(512, 0));
        }

        // show the modal
        if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return nullptr;  // the modal is not currently showing
        }

        T const* selected = nullptr;

        // iterate through each T in `root` and give the user the option to click it
        {
            ImGui::BeginChild("first", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (T const& c : root.getComponentList<T>()) {
                if (ImGui::Button(c.getName().c_str())) {
                    selected = &c;
                }
            }
            ImGui::EndChild();
        }

        // close modal if something is selected or the user presses cancel
        if (selected || ImGui::Button("cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();

        return selected;
    }
}
