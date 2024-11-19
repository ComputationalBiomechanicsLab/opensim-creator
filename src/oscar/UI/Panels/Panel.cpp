#include "Panel.h"

#include <oscar/Platform/App.h>
#include <oscar/UI/Panels/PanelPrivate.h>

#include <memory>
#include <string_view>
#include <utility>

osc::Panel::Panel() :
    Panel{std::make_unique<PanelPrivate>(*this)}
{}
osc::Panel::Panel(Widget* parent, std::string_view panel_name, ui::WindowFlags window_flags) :
    Panel{std::make_unique<PanelPrivate>(*this, parent, panel_name, window_flags)}
{}
osc::Panel::Panel(std::unique_ptr<PanelPrivate>&& ptr) :
    Widget{std::move(ptr)}
{}

bool osc::Panel::is_open() const
{
    return private_data().is_open();
}

void osc::Panel::open()
{
    private_data().set_open(true);
}

void osc::Panel::close()
{
    private_data().set_open(false);
}

void osc::Panel::impl_on_draw()
{
    if (is_open()) {
        bool open = true;

        impl_before_imgui_begin();
        const bool began = ui::begin_panel(private_data().name(), &open, private_data().panel_flags());
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
