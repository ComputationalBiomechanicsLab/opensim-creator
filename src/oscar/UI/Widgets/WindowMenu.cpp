#include "WindowMenu.hpp"

#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <imgui.h>

#include <memory>
#include <utility>

osc::WindowMenu::WindowMenu(std::shared_ptr<PanelManager> panelManager) :
    m_PanelManager{std::move(panelManager)}
{
}
osc::WindowMenu::WindowMenu(WindowMenu&&) noexcept = default;
osc::WindowMenu& osc::WindowMenu::operator=(WindowMenu&&) noexcept = default;
osc::WindowMenu::~WindowMenu() = default;

void osc::WindowMenu::onDraw()
{
    if (ImGui::BeginMenu("Window"))
    {
        drawContent();
        ImGui::EndMenu();
    }
}

void osc::WindowMenu::drawContent()
{
    osc::PanelManager& manager = *m_PanelManager;

    size_t numMenuItemsPrinted = 0;

    // toggleable panels
    for (size_t i = 0; i < manager.getNumToggleablePanels(); ++i)
    {
        bool activated = manager.isToggleablePanelActivated(i);
        osc::CStringView const name = manager.getToggleablePanelName(i);
        if (ImGui::MenuItem(name.c_str(), nullptr, &activated))
        {
            manager.setToggleablePanelActivated(i, activated);
        }
        ++numMenuItemsPrinted;
    }

    // dynamic panels
    if (manager.getNumDynamicPanels() > 0)
    {
        ImGui::Separator();
        for (size_t i = 0; i < manager.getNumDynamicPanels(); ++i)
        {
            bool activated = true;
            osc::CStringView const name = manager.getDynamicPanelName(i);
            if (ImGui::MenuItem(name.c_str(), nullptr, &activated))
            {
                manager.deactivateDynamicPanel(i);
            }
            ++numMenuItemsPrinted;
        }
    }

    // spawnable submenu
    if (manager.getNumSpawnablePanels() > 0)
    {
        ImGui::Separator();

        if (ImGui::BeginMenu("Add"))
        {
            for (size_t i = 0; i < manager.getNumSpawnablePanels(); ++i)
            {
                osc::CStringView const name = manager.getSpawnablePanelBaseName(i);
                if (ImGui::MenuItem(name.c_str()))
                {
                    manager.createDynamicPanel(i);
                }
            }
            ImGui::EndMenu();
            ++numMenuItemsPrinted;
        }
    }

    if (numMenuItemsPrinted <= 0)
    {
        ImGui::TextDisabled("(no windows available to be toggled)");
    }
}
