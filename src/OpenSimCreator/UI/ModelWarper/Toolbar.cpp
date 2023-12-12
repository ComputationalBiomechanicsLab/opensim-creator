#include "Toolbar.hpp"

#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>

#include <imgui.h>

#include <utility>

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
    DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
    {
        m_State->actionOpenModel(std::move(maybeSelection));
    });
}
