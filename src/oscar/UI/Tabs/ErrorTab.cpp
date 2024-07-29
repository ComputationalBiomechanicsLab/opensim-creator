#include "ErrorTab.h"

#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/Widgets/LogViewer.h>

#include <IconsFontAwesome5.h>

#include <exception>
#include <memory>
#include <string>

using namespace osc;

class osc::ErrorTab::Impl final : public StandardTabImpl {
public:
    explicit Impl(const std::exception& exception) :
        StandardTabImpl{ICON_FA_SPIDER " Error"},
        error_message_{exception.what()}
    {}

private:
    void impl_on_draw() final
    {
        constexpr float width = 800.0f;
        constexpr float padding = 10.0f;

        const Rect viewport_ui_rect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_ui_rect);

        // error message panel
        {
            Vec2 pos{viewport_ui_rect.p1.x + viewport_dimensions.x/2.0f, viewport_ui_rect.p1.y + padding};
            ui::set_next_panel_pos(pos, ImGuiCond_Once, {0.5f, 0.0f});
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
            Vec2 pos{viewport_ui_rect.p1.x + viewport_dimensions.x/2.0f, viewport_ui_rect.p2.y - padding};
            ui::set_next_panel_pos(pos, ImGuiCond_Once, {0.5f, 1.0f});
            ui::set_next_panel_size({width, 0.0f});

            if (ui::begin_panel("Error Log", nullptr, ImGuiWindowFlags_MenuBar)) {
                log_viewer_.on_draw();
            }
            ui::end_panel();
        }
    }

    std::string error_message_;
    LogViewer log_viewer_;
};

osc::ErrorTab::ErrorTab(const ParentPtr<ITabHost>&, const std::exception& exception) :
    impl_{std::make_unique<Impl>(exception)}
{}
osc::ErrorTab::ErrorTab(ErrorTab&&) noexcept = default;
osc::ErrorTab& osc::ErrorTab::operator=(ErrorTab&&) noexcept = default;
osc::ErrorTab::~ErrorTab() noexcept = default;

UID osc::ErrorTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::ErrorTab::impl_get_name() const
{
    return impl_->name();
}

void osc::ErrorTab::impl_on_draw()
{
    impl_->on_draw();
}
