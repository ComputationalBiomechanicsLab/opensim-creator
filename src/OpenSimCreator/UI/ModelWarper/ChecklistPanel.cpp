#include "ChecklistPanel.hpp"

#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>

void osc::mow::ChecklistPanel::implDrawContent()
{
    ImGui::Text("Meshes");
    ImGui::SameLine();
    DrawHelpMarker("Shows which meshes are present in the source model, whether they have associated landmarks, and whether there is a known destination mesh");

    ImGui::Separator();
    for (int i = 0; i < 4; ++i)
    {
        ImGui::Text("Mesh %i", i);
        for (int j = 0; j < (i+1)*2; ++j)
        {
            ImGui::Text("    Landmark %i", j);
        }
    }
    ImGui::NewLine();

    ImGui::Text("Frames");
    ImGui::SameLine();
    DrawHelpMarker("Shows frames in the source model, and whether they are warp-able or not");

    ImGui::Separator();
    for (int i = 0; i < 6; ++i)
    {
        ImGui::Text("Frame %i", i);
    }
}
