#include "FileMenu.h"

#include <oscar/UI/oscimgui.h>

void osc::mow::FileMenu::onDraw()
{
    if (ImGui::BeginMenu("File")) {
        drawContent();
        ImGui::EndMenu();
    }
}

void osc::mow::FileMenu::drawContent()
{
    ui::Text("TODO");
}
