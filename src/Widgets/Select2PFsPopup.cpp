#include "Select2PFsPopup.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <string>

std::optional<osc::Select2PFsPopup::Response> osc::Select2PFsPopup::draw(
    char const* popupName,
    OpenSim::Model const& model,
    char const* firstLabel,
    char const* secondLabel)
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
        return std::nullopt;
    }

    ImGui::Columns(2);

    ImGui::Text("%s", firstLabel);
    ImGui::BeginChild("first", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>())
    {
        if (&b == second)
        {
            continue;  // don't allow circular connections
        }

        int numStylesPushed = 0;
        if (&b == first)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.3f, 1.0f});
            ++numStylesPushed;
        }
        if (ImGui::Selectable(b.getName().c_str()))
        {
            first = &b;
        }
        ImGui::PopStyleColor(numStylesPushed);
    }
    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Text("%s", secondLabel);
    ImGui::BeginChild("second", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>())
    {
        if (&b == first)
        {
            continue;  // don't allow circular connections
        }

        int numStylesPushed = 0;
        if (&b == second)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.3f, 1.0f});
            ++numStylesPushed;
        }
        if (ImGui::Selectable(b.getName().c_str()))
        {
            second = &b;
        }
        ImGui::PopStyleColor(numStylesPushed);
    }
    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Columns(1);

    std::optional<Response> rv = std::nullopt;

    if (first && second)
    {
        if (ImGui::Button("OK"))
        {
            rv.emplace(*first, *second);
            *this = {};  // reset user inputs
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
    }

    if (ImGui::Button("cancel"))
    {
        *this = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}
