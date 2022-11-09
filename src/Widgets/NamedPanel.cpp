#include "NamedPanel.hpp"

#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

osc::NamedPanel::NamedPanel(std::string_view name) :
    NamedPanel{std::move(name), ImGuiWindowFlags_None}
{
}

osc::NamedPanel::NamedPanel(std::string_view name, int imGuiWindowFlags) :
    m_PanelName{std::move(name)},
    m_PanelFlags{std::move(imGuiWindowFlags)}
{
}

bool osc::NamedPanel::implIsOpen() const
{
    return osc::App::get().getConfig().getIsPanelEnabled(m_PanelName);
}

void osc::NamedPanel::implOpen()
{
    osc::App::upd().updConfig().setIsPanelEnabled(m_PanelName, true);
}

void osc::NamedPanel::implClose()
{
    osc::App::upd().updConfig().setIsPanelEnabled(m_PanelName, false);
}

void osc::NamedPanel::implDraw()
{
    if (isOpen())
    {
        bool v = true;
        implBeforeImGuiBegin();
        if (ImGui::Begin(m_PanelName.c_str(), &v, m_PanelFlags))
        {
            implAfterImGuiBegin();
            implDrawContent();
        }
        else
        {
            implAfterImGuiBegin();
        }
        ImGui::End();

        if (!v)
        {
            close();
        }
    }
}

void osc::NamedPanel::requestClose()
{
    close();
}
