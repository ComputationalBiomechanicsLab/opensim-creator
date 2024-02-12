#include "FileMenu.h"

#include <imgui.h>

void osc::mow::FileMenu::onDraw()
{
    if (ImGui::BeginMenu("File"))
    {
        drawContent();
        ImGui::EndMenu();
    }
}

void osc::mow::FileMenu::drawContent()
{
    ImGui::Text("TODO");
}
