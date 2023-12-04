#include "Toolbar.hpp"

#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>

#include <imgui.h>

void osc::mow::Toolbar::onDraw()
{
    if (BeginToolbar(m_Label))
    {
        drawContent();
    }
    ImGui::End();
}

void osc::mow::Toolbar::drawContent()
{
    ImGui::Text("todo");
}
