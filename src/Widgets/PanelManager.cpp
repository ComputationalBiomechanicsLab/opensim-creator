#include "PanelManager.hpp"

#include "src/Widgets/ToggleablePanel.hpp"

#include <nonstd/span.hpp>

#include <vector>

nonstd::span<osc::ToggleablePanel> osc::PanelManager::updToggleablePanels()
{
    return m_Panels;
}

void osc::PanelManager::push_back(ToggleablePanel&& panel)
{
    m_Panels.push_back(std::move(panel));
}

void osc::PanelManager::activateAllDefaultOpenPanels()
{
    // initialize default-open tabs
    for (ToggleablePanel& panel : m_Panels)
    {
        if (panel.isEnabledByDefault())
        {
            panel.activate();
        }
    }
}

void osc::PanelManager::garbageCollectDeactivatedPanels()
{
    // garbage collect closed-panel instance data
    for (ToggleablePanel& panel : m_Panels)
    {
        panel.garbageCollect();
    }
}

void osc::PanelManager::drawAllActivatedPanels()
{
    for (ToggleablePanel& panel : m_Panels)
    {
        if (panel.isActivated())
        {
            panel.draw();
        }
    }
}
