#include "LogViewerPanel.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/UI/Widgets/LogViewer.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::LogViewerPanel::Impl final : public StandardPanelImpl {
public:

    explicit Impl(std::string_view panelName) :
        StandardPanelImpl{panelName, ImGuiWindowFlags_MenuBar}
    {
    }

private:
    void implDrawContent() final
    {
        m_Viewer.onDraw();
    }

    LogViewer m_Viewer;
};

osc::LogViewerPanel::LogViewerPanel(std::string_view panelName) :
    m_Impl{std::make_unique<Impl>(panelName)}
{
}
osc::LogViewerPanel::LogViewerPanel(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel& osc::LogViewerPanel::operator=(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel::~LogViewerPanel() noexcept = default;

CStringView osc::LogViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::LogViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::LogViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::LogViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::LogViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}
