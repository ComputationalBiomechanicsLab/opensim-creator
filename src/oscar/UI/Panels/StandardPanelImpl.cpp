#include "StandardPanelImpl.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/UI/oscimgui.h>

#include <string_view>

using namespace osc;

osc::StandardPanelImpl::StandardPanelImpl(std::string_view panel_name) :
    StandardPanelImpl{panel_name, ImGuiWindowFlags_None}
{}

osc::StandardPanelImpl::StandardPanelImpl(
    std::string_view panel_name,
    ImGuiWindowFlags panel_flags) :

    panel_name_{panel_name},
    panel_flags_{panel_flags}
{}

CStringView osc::StandardPanelImpl::impl_get_name() const
{
    return panel_name_;
}

bool osc::StandardPanelImpl::impl_is_open() const
{
    return App::config().is_panel_enabled(panel_name_);
}

void osc::StandardPanelImpl::impl_open()
{
    App::upd().upd_config().set_panel_enabled(panel_name_, true);
}

void osc::StandardPanelImpl::impl_close()
{
    App::upd().upd_config().set_panel_enabled(panel_name_, false);
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
