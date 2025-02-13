#include "ErrorTab.h"

#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/UI/Widgets/LogViewer.h>

#include <exception>
#include <memory>
#include <string>

using namespace osc;

class osc::ErrorTab::Impl final : public TabPrivate {
public:
    explicit Impl(ErrorTab& owner, Widget& parent, const std::exception& exception) :
        TabPrivate{owner, &parent, OSC_ICON_SPIDER " Error"},
        error_message_{exception.what()}
    {}

    void on_draw()
    {
        constexpr float width = 800.0f;
        constexpr float padding = 10.0f;

        const Rect viewport_ui_rect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_ui_rect);

        // error message panel
        {
            const Vec2 pos{viewport_ui_rect.p1.x + viewport_dimensions.x/2.0f, viewport_ui_rect.p1.y + padding};
            ui::set_next_panel_pos(pos, ui::Conditional::Once, {0.5f, 0.0f});
            ui::set_next_panel_size({width, 0.0f});

            if (ui::begin_panel("fatal error")) {
                ui::draw_text_wrapped("The application threw an exception with the following message:");
                ui::draw_dummy({2.0f, 10.0f});
                ui::same_line();
                ui::draw_text_wrapped(error_message_);
                ui::draw_dummy({0.0f, 10.0f});
            }
            ui::end_panel();
        }

        // log message panel
        {
            const Vec2 pos{viewport_ui_rect.p1.x + viewport_dimensions.x/2.0f, viewport_ui_rect.p2.y - padding};
            ui::set_next_panel_pos(pos, ui::Conditional::Once, {0.5f, 1.0f});
            ui::set_next_panel_size({width, 0.0f});

            if (ui::begin_panel("Error Log", nullptr, ui::PanelFlag::MenuBar)) {
                log_viewer_.on_draw();
            }
            ui::end_panel();
        }
    }

private:
    std::string error_message_;
    LogViewer log_viewer_;
};

osc::ErrorTab::ErrorTab(Widget& parent, const std::exception& exception) :
    Tab{std::make_unique<Impl>(*this, parent, exception)}
{}
void osc::ErrorTab::impl_on_draw() { private_data().on_draw(); }
