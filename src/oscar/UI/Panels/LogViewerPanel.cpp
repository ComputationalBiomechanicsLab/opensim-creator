#include "LogViewerPanel.h"

#include <oscar/UI/Panels/PanelPrivate.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/LogViewer.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::LogViewerPanel::Impl final : public PanelPrivate {
public:

    explicit Impl(LogViewerPanel& owner, std::string_view panel_name) :
        PanelPrivate{owner, nullptr, panel_name, ui::WindowFlag::MenuBar}
    {}

    void draw_content() { log_viewer_.on_draw(); }

private:
    LogViewer log_viewer_;
};

osc::LogViewerPanel::LogViewerPanel(std::string_view panel_name) :
    Panel{std::make_unique<Impl>(*this, panel_name)}
{}
void osc::LogViewerPanel::impl_draw_content() { private_data().draw_content(); }
