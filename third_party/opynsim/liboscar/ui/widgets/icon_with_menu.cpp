#include "icon_with_menu.h"

#include <liboscar/ui/icon.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/widgets/icon_without_menu.h>
#include <liboscar/utilities/c_string_view.h>

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
        ui::draw_vertical_spacer(0.5f);
        rv = content_renderer_();
        ui::end_popup();
    }

    return rv;
}
