#include "WindowMenu.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <utility>

osc::WindowMenu::WindowMenu(std::shared_ptr<PanelManager> panelManager) :
    m_PanelManager{std::move(panelManager)}
{
}
osc::WindowMenu::WindowMenu(WindowMenu&&) noexcept = default;
osc::WindowMenu& osc::WindowMenu::operator=(WindowMenu&&) noexcept = default;
osc::WindowMenu::~WindowMenu() noexcept = default;

void osc::WindowMenu::onDraw()
{
    if (ui::BeginMenu("Window"))
    {
        drawContent();
        ui::EndMenu();
    }
}

void osc::WindowMenu::drawContent()
{
    PanelManager& manager = *m_PanelManager;

    size_t numMenuItemsPrinted = 0;

    // toggleable panels
    for (size_t i = 0; i < manager.getNumToggleablePanels(); ++i)
    {
        bool activated = manager.isToggleablePanelActivated(i);
        CStringView const name = manager.getToggleablePanelName(i);
        if (ui::MenuItem(name.c_str(), nullptr, &activated))
        {
            manager.setToggleablePanelActivated(i, activated);
        }
        ++numMenuItemsPrinted;
    }

    // dynamic panels
    if (manager.getNumDynamicPanels() > 0)
    {
        ui::Separator();
        for (size_t i = 0; i < manager.getNumDynamicPanels(); ++i)
        {
            bool activated = true;
            CStringView const name = manager.getDynamicPanelName(i);
            if (ui::MenuItem(name.c_str(), nullptr, &activated))
            {
                manager.deactivateDynamicPanel(i);
            }
            ++numMenuItemsPrinted;
        }
    }

    // spawnable submenu
    if (manager.getNumSpawnablePanels() > 0)
    {
        ui::Separator();

        if (ui::BeginMenu("Add"))
        {
            for (size_t i = 0; i < manager.getNumSpawnablePanels(); ++i)
            {
                CStringView const name = manager.getSpawnablePanelBaseName(i);
                if (ui::MenuItem(name.c_str()))
                {
                    manager.createDynamicPanel(i);
                }
            }
            ui::EndMenu();
            ++numMenuItemsPrinted;
        }
    }

    if (numMenuItemsPrinted <= 0)
    {
        ui::TextDisabled("(no windows available to be toggled)");
    }
}
