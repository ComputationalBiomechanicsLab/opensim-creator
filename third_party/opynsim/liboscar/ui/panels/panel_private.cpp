#include "panel_private.h"

#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/utilities/conversion.h>

#include <format>
#include <string>
#include <string_view>

osc::PanelPrivate::PanelPrivate(
    Panel& owner,
    Widget* parent,
    std::string_view panel_name,
    ui::PanelFlags panel_flags) :

    WidgetPrivate{owner, parent},
    panel_enabled_config_key_{std::format("panels/{}/enabled", panel_name)},
    panel_flags_{panel_flags}
{
    set_name(panel_name);
}

bool osc::PanelPrivate::is_open() const
{
    if (auto v = App::settings().find_value(panel_enabled_config_key_)) {
        return to<bool>(*v);
    }
    else {
        return false;
    }
}

void osc::PanelPrivate::set_open(bool v)
{
    App::upd_settings().set_value(panel_enabled_config_key_, v);
}
