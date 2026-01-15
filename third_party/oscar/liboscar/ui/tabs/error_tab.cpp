#include "error_tab.h"

#include <liboscar/maths/MathHelpers.h>
#include <liboscar/maths/Rect.h>
#include <liboscar/maths/RectFunctions.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/log_viewer.h>

#include <exception>
#include <memory>
#include <string>

using namespace osc;

class osc::ErrorTab::Impl final : public TabPrivate {
public:
    explicit Impl(ErrorTab& owner, Widget& parent, const std::exception& exception) :
        TabPrivate{owner, &parent, "Error!"},
        error_message_{exception.what()}
    {}

    void on_draw()
    {
        constexpr float width = 800.0f;
        constexpr float padding = 10.0f;

        const Rect workspace_ui_rect = ui::get_main_window_workspace_ui_rect();
        const Vector2 workspace_dimensions = workspace_ui_rect.dimensions();
        const Vector2 workspace_ui_top_left = workspace_ui_rect.ypd_top_left();

        // error message panel
        {
            const Vector2 ui_pos{workspace_ui_top_left.x + workspace_dimensions.x/2.0f, workspace_ui_top_left.y + padding};
            ui::set_next_panel_ui_position(ui_pos, ui::Conditional::Once, {0.5f, 0.0f});
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
            const Vector2 ui_pos{workspace_ui_top_left.x + workspace_dimensions.x/2.0f, workspace_ui_top_left.y - padding};
            ui::set_next_panel_ui_position(ui_pos, ui::Conditional::Once, {0.5f, 1.0f});
            ui::set_next_panel_size({width, 0.0f});

            if (ui::begin_panel("Error Log", nullptr, ui::PanelFlag::MenuBar)) {
                log_viewer_.on_draw();
            }
            ui::end_panel();
        }
    }

private:
    std::string error_message_;
    LogViewer log_viewer_{&owner()};
};

osc::ErrorTab::ErrorTab(Widget& parent, const std::exception& exception) :
    Tab{std::make_unique<Impl>(*this, parent, exception)}
{}
void osc::ErrorTab::impl_on_draw() { private_data().on_draw(); }
