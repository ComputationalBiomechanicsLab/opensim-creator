#include "StandardPanelImpl.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/UI/oscimgui.h>

#include <string_view>

using namespace osc;

osc::StandardPanelImpl::StandardPanelImpl(std::string_view panelName) :
    StandardPanelImpl{panelName, ImGuiWindowFlags_None}
{
}

osc::StandardPanelImpl::StandardPanelImpl(
    std::string_view panelName,
    ImGuiWindowFlags imGuiWindowFlags) :

    m_PanelName{panelName},
    m_PanelFlags{imGuiWindowFlags}
{
}

CStringView osc::StandardPanelImpl::implGetName() const
{
    return m_PanelName;
}

bool osc::StandardPanelImpl::implIsOpen() const
{
    return App::get().getConfig().getIsPanelEnabled(m_PanelName);
}

void osc::StandardPanelImpl::implOpen()
{
    App::upd().updConfig().setIsPanelEnabled(m_PanelName, true);
}

void osc::StandardPanelImpl::implClose()
{
    App::upd().updConfig().setIsPanelEnabled(m_PanelName, false);
}

void osc::StandardPanelImpl::implOnDraw()
{
    if (isOpen())
    {
        bool v = true;
        implBeforeImGuiBegin();
        bool began = ui::Begin(m_PanelName, &v, m_PanelFlags);
        implAfterImGuiBegin();
        if (began)
        {
            implDrawContent();
        }
        ui::End();

        if (!v)
        {
            close();
        }
    }
}

void osc::StandardPanelImpl::requestClose()
{
    close();
}
