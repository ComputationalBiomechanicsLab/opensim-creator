#include "StandardPanel.hpp"

#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

osc::StandardPanel::StandardPanel(std::string_view name) :
    StandardPanel{std::move(name), ImGuiWindowFlags_None}
{
}

osc::StandardPanel::StandardPanel(std::string_view name, ImGuiWindowFlags imGuiWindowFlags) :
    m_PanelName{std::move(name)},
    m_PanelFlags{std::move(imGuiWindowFlags)}
{
}

bool osc::StandardPanel::implIsOpen() const
{
    return osc::App::get().getConfig().getIsPanelEnabled(m_PanelName);
}

void osc::StandardPanel::implOpen()
{
    osc::App::upd().updConfig().setIsPanelEnabled(m_PanelName, true);
}

void osc::StandardPanel::implClose()
{
    osc::App::upd().updConfig().setIsPanelEnabled(m_PanelName, false);
}

void osc::StandardPanel::implDraw()
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

void osc::StandardPanel::requestClose()
{
    close();
}
