#include "LogViewerPanel.hpp"

#include "oscar/UI/Panels/StandardPanel.hpp"
#include "oscar/UI/Widgets/LogViewer.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

class osc::LogViewerPanel::Impl final : public StandardPanel {
public:

    explicit Impl(std::string_view panelName) :
        StandardPanel{panelName, ImGuiWindowFlags_MenuBar}
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

osc::CStringView osc::LogViewerPanel::implGetName() const
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
