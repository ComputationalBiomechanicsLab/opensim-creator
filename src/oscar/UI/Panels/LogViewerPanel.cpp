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
        StandardPanelImpl{panel_name, ui::WindowFlag::MenuBar}
    {}

private:
    void impl_draw_content() final
    {
        log_viewer_.on_draw();
    }

    LogViewer log_viewer_;
};

osc::LogViewerPanel::LogViewerPanel(std::string_view panel_name) :
    impl_{std::make_unique<Impl>(panel_name)}
{}
osc::LogViewerPanel::LogViewerPanel(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel& osc::LogViewerPanel::operator=(LogViewerPanel&&) noexcept = default;
osc::LogViewerPanel::~LogViewerPanel() noexcept = default;

CStringView osc::LogViewerPanel::impl_get_name() const
{
    return impl_->name();
}

bool osc::LogViewerPanel::impl_is_open() const
{
    return impl_->is_open();
}

void osc::LogViewerPanel::impl_open()
{
    impl_->open();
}

void osc::LogViewerPanel::impl_close()
{
    impl_->close();
}

void osc::LogViewerPanel::impl_on_draw()
{
    impl_->on_draw();
}
