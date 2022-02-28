#include "Select1PFPopup.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

OpenSim::PhysicalFrame const* osc::Select1PFPopup::draw(
    char const* popupName,
    OpenSim::Model const& model,
    nonstd::span<OpenSim::PhysicalFrame const*> exclusions)
{
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // modal not showing
        return nullptr;
    }

    // list the PFs
    OpenSim::PhysicalFrame const* selected = nullptr;

    ImGui::BeginChild("pflist", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto const& pf : model.getComponentList<OpenSim::PhysicalFrame>())
    {
        bool excluded = std::find(exclusions.begin(), exclusions.end(), &pf) != exclusions.end();

        if (excluded)
        {
            continue;
        }

        if (ImGui::Selectable(pf.getName().c_str()))
        {
            selected = &pf;
        }
    }
    ImGui::EndChild();

    if (selected)
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    
    return selected;
}
