#include "ToggleablePanels.hpp"

#include "src/Widgets/ToggleablePanel.hpp"

#include <nonstd/span.hpp>

#include <vector>

nonstd::span<osc::ToggleablePanel> osc::ToggleablePanels::upd()
{
    return m_Panels;
}

void osc::ToggleablePanels::push_back(ToggleablePanel&& panel)
{
    m_Panels.push_back(std::move(panel));
}

void osc::ToggleablePanels::activateAllDefaultOpenPanels()
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

void osc::ToggleablePanels::garbageCollectDeactivatedPanels()
{
    // garbage collect closed-panel instance data
    for (ToggleablePanel& panel : m_Panels)
    {
        panel.garbageCollect();
    }
}

void osc::ToggleablePanels::drawAllActivatedPanels()
{
    for (ToggleablePanel& panel : m_Panels)
    {
        if (panel.isActivated())
        {
            panel.draw();
        }
    }
}
