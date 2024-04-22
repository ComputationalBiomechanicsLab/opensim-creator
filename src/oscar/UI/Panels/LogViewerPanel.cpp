#include "LogViewerPanel.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/UI/Widgets/LogViewer.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::LogViewerPanel::Impl final : public StandardPanelImpl {
public:

    explicit Impl(std::string_view panel_name) :
        StandardPanelImpl{panel_name, ImGuiWindowFlags_MenuBar}
    {}

private:
    void impl_draw_content() final
    {
        log_viewer_.onDraw();
    }

    LogViewer log_viewer_;
};

osc::LogViewerPanel::LogViewerPanel(std::string_view panel_name) :
    m_Impl{std::make_unique<Impl>(panel_name)}
{
}
osc::LogViewerPanel::LogViewerPanel(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel& osc::LogViewerPanel::operator=(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel::~LogViewerPanel() noexcept = default;

CStringView osc::LogViewerPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::LogViewerPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::LogViewerPanel::impl_open()
{
    m_Impl->open();
}

void osc::LogViewerPanel::impl_close()
{
    m_Impl->close();
}

void osc::LogViewerPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
