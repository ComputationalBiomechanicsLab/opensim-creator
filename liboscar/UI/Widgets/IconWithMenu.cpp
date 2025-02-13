#include "IconWithMenu.h"

#include <liboscar/UI/Icon.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Widgets/IconWithoutMenu.h>
#include <liboscar/Utils/CStringView.h>

#include <functional>
#include <utility>

osc::IconWithMenu::IconWithMenu(
    Icon icon,
    CStringView title,
    CStringView description,
    std::function<bool()> content_renderer) :

    icon_without_menu_{std::move(icon), title, description},
    context_menu_id_{"##" + icon_without_menu_.icon_id()},
    content_renderer_{std::move(content_renderer)}
{}

bool osc::IconWithMenu::on_draw()
{
    if (icon_without_menu_.on_draw()) {
        ui::open_popup(context_menu_id_);
    }

    bool rv = false;
    if (ui::begin_popup(context_menu_id_, {ui::PanelFlag::AlwaysAutoResize, ui::PanelFlag::NoTitleBar, ui::PanelFlag::NoSavedSettings})) {
        ui::draw_text_disabled(icon_without_menu_.title());
        ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});
        rv = content_renderer_();
        ui::end_popup();
    }

    return rv;
}
