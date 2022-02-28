#include <OpenSim/Common/Component.h>
#include <imgui.h>

// popup for selecting a component of the specified type
namespace osc
{
    template<typename T>
    struct SelectComponentPopup final {

        // returns non-nullptr if user selects a component of type T
        //
        // expects the caller to handle ImGui::OpenPopup
        T const* draw(char const* popupName, OpenSim::Component const& root)
        {
            // center the modal
            {
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(512, 0));
            }

            // show the modal
            if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                return nullptr;  // the modal is not currently showing
            }

            T const* selected = nullptr;

            // iterate through each T in `root` and give the user the option to click it
            {
                ImGui::BeginChild("first", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
                for (T const& c : root.getComponentList<T>())
                {
                    if (ImGui::Button(c.getName().c_str()))
                    {
                        selected = &c;
                    }
                }
                ImGui::EndChild();
            }

            // close modal if something is selected or the user presses cancel
            if (selected || ImGui::Button("cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();

            return selected;
        }
    };
}
