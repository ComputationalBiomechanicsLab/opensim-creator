#include "StandardPanel.hpp"

#include "oscar/Platform/App.hpp"
#include "oscar/Platform/Config.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

osc::StandardPanel::StandardPanel(std::string_view panelName) :
    StandardPanel{panelName, ImGuiWindowFlags_None}
{
}

osc::StandardPanel::StandardPanel(
    std::string_view panelName,
    ImGuiWindowFlags imGuiWindowFlags) :

    m_PanelName{panelName},
    m_PanelFlags{imGuiWindowFlags}
{
}

osc::CStringView osc::StandardPanel::implGetName() const
{
    return m_PanelName;
}

bool osc::StandardPanel::implIsOpen() const
{
    return App::get().getConfig().getIsPanelEnabled(m_PanelName);
}

void osc::StandardPanel::implOpen()
{
    App::upd().updConfig().setIsPanelEnabled(m_PanelName, true);
}

void osc::StandardPanel::implClose()
{
    App::upd().updConfig().setIsPanelEnabled(m_PanelName, false);
}

void osc::StandardPanel::implOnDraw()
{
    if (isOpen())
    {
        bool v = true;
        implBeforeImGuiBegin();
        bool began = ImGui::Begin(m_PanelName.c_str(), &v, m_PanelFlags);
        implAfterImGuiBegin();
        if (began)
        {
            implDrawContent();
        }
        ImGui::End();

        if (!v)
        {
            close();
        }
    }
}

void osc::StandardPanel::requestClose()
{
    close();
}
