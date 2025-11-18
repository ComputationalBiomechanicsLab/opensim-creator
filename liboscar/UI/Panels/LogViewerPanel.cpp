#include "LogViewerPanel.h"

#include <liboscar/UI/Panels/PanelPrivate.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Widgets/LogViewer.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::LogViewerPanel::Impl final : public PanelPrivate {
public:

    explicit Impl(LogViewerPanel& owner, Widget* parent, std::string_view panel_name) :
        PanelPrivate{owner, parent, panel_name, ui::PanelFlag::MenuBar}
    {}

    void draw_content() { log_viewer_.on_draw(); }

private:
    LogViewer log_viewer_{&owner()};
};

osc::LogViewerPanel::LogViewerPanel(Widget* parent, std::string_view panel_name) :
    Panel{std::make_unique<Impl>(*this, parent, panel_name)}
{}
void osc::LogViewerPanel::impl_draw_content() { private_data().draw_content(); }
