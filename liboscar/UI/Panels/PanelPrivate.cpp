#include "PanelPrivate.h"

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/Utils/Conversion.h>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    std::string create_panel_enabled_config_key(std::string_view panel_name)
    {
        std::stringstream ss;
        ss << "panels/" << panel_name << "/enabled";
        return std::move(ss).str();
    }
}

osc::PanelPrivate::PanelPrivate(
    Panel& owner,
    Widget* parent,
    std::string_view panel_name,
    ui::PanelFlags panel_flags) :

    WidgetPrivate{owner, parent},
    panel_enabled_config_key_{create_panel_enabled_config_key(panel_name)},
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
    App::upd().upd_settings().set_value(panel_enabled_config_key_, v);
}
