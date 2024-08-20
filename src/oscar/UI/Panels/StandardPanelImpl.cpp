#include "StandardPanelImpl.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Variant/Variant.h>

#include <sstream>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    std::string create_panel_enabled_config_key(std::string_view panel_name)
    {
        std::stringstream ss;
        ss << "panels/" << panel_name << "/enabled";
        return std::move(ss).str();
    }
}

osc::StandardPanelImpl::StandardPanelImpl(std::string_view panel_name) :
    StandardPanelImpl{panel_name, ImGuiWindowFlags_None}
{}

osc::StandardPanelImpl::StandardPanelImpl(
    std::string_view panel_name,
    ImGuiWindowFlags panel_flags) :

    panel_name_{panel_name},
    panel_enabled_config_key_{create_panel_enabled_config_key(panel_name_)},
    panel_flags_{panel_flags}
{}

CStringView osc::StandardPanelImpl::impl_get_name() const
{
    return panel_name_;
}

bool osc::StandardPanelImpl::impl_is_open() const
{
    if (auto v = App::settings().find_value(panel_enabled_config_key_)) {
        return to<bool>(*v);
    }
    else {
        return false;
    }
}

void osc::StandardPanelImpl::impl_open()
{
    App::upd().upd_settings().set_value(panel_enabled_config_key_, true);
}

void osc::StandardPanelImpl::impl_close()
{
    App::upd().upd_settings().set_value(panel_enabled_config_key_, false);
}

void osc::StandardPanelImpl::impl_on_draw()
{
    if (is_open()) {
        bool open = true;

        impl_before_imgui_begin();
        const bool began = ui::begin_panel(panel_name_, &open, panel_flags_);
        impl_after_imgui_begin();

        if (began) {
            impl_draw_content();
        }
        ui::end_panel();

        if (not open) {
            close();
        }
    }
}

void osc::StandardPanelImpl::request_close()
{
    close();
}
