#include "Popup.h"

#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/App.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Popups/PopupPrivate.h>

#include <memory>
#include <optional>
#include <string_view>

osc::PopupPrivate::PopupPrivate(
    Popup& owner,
    Widget* parent,
    std::string_view name,
    Vec2 dimensions,
    ui::PanelFlags panel_flags) :

    WidgetPrivate(owner, parent),
    dimensions_{dimensions},
    maybe_position_{std::nullopt},
    panel_flags_{panel_flags},
    should_open_{false},
    should_close_{false},
    just_opened_{false},
    is_open_{false},
    is_modal_{true}
{
    set_name(name);
}

bool osc::PopupPrivate::is_popup_opened_this_frame() const
{
    return just_opened_;
}

void osc::PopupPrivate::request_close()
{
    should_close_ = true;
    should_open_ = false;
}

void osc::PopupPrivate::set_modal(bool v)
{
    is_modal_ = v;
}

osc::Popup::Popup(
    Widget* parent,
    std::string_view name,
    Vec2 dimensions,
    ui::PanelFlags panel_flags) :

    Popup{std::make_unique<PopupPrivate>(*this, parent, name, dimensions, panel_flags)}
{}
osc::Popup::Popup(std::unique_ptr<PopupPrivate>&& pimpl) :
    Widget{std::move(pimpl)}
{}

bool osc::Popup::is_open()
{
    PopupPrivate& pimpl = private_data();
    return pimpl.should_open_ or pimpl.is_open_;
}

void osc::Popup::open()
{
    PopupPrivate& pimpl = private_data();
    pimpl.should_open_ = true;
    pimpl.should_close_ = false;
}

void osc::Popup::close()
{
    PopupPrivate& pimpl = private_data();
    pimpl.should_close_ = true;
    pimpl.should_open_ = false;
}

bool osc::Popup::begin_popup()
{
    PopupPrivate& pimpl = private_data();

    if (pimpl.should_open_) {
        ui::open_popup(name());
        pimpl.should_open_ = false;
        pimpl.should_close_ = false;
        pimpl.just_opened_ = true;
    }

    if (pimpl.is_modal_) {
        // if specified, set the position of the modal upon appearing
        //
        // else, position the modal in the center of the application window
        if (pimpl.maybe_position_) {
            ui::set_next_panel_ui_position(
                static_cast<Vec2>(*pimpl.maybe_position_),
                ui::Conditional::Appearing
            );
        }
        else {
            ui::set_next_panel_ui_position(
                0.5f*App::get().main_window_dimensions(),
                ui::Conditional::Appearing,
                Vec2{0.5f, 0.5f}
            );
        }

        // if the modal doesn't auto-resize each frame, then set the size of
        // modal only upon appearing
        //
        // else, set the position every frame, because the __nonzero__ dimensions
        // will stretch out the modal accordingly
        if (not (pimpl.panel_flags_ & ui::PanelFlag::AlwaysAutoResize)) {
            ui::set_next_panel_size(
                Vec2{pimpl.dimensions_},
                ui::Conditional::Appearing
            );
        }
        else {
            ui::set_next_panel_size(
                Vec2{pimpl.dimensions_}
            );
        }

        // try to begin the modal window
        impl_before_imgui_begin_popup();
        const bool opened = ui::begin_popup_modal(name(), nullptr, pimpl.panel_flags_);
        impl_after_imgui_begin_popup();

        if (not opened) {
            // modal not showing
            pimpl.is_open_ = false;
            return false;
        }
    }
    else {
        // if specified, set the position of the modal upon appearing
        //
        // else, do nothing - the popup's position will be determined
        // by other means (unlike a modal, which usually takes control
        // of the ui and, therefore, should probably be centered
        // in it)
        if (pimpl.maybe_position_) {
            ui::set_next_panel_ui_position(
                static_cast<Vec2>(*pimpl.maybe_position_),
                ui::Conditional::Appearing
            );
        }

        // try to begin the popup window
        impl_before_imgui_begin_popup();
        const bool opened = ui::begin_popup(name(), pimpl.panel_flags_);
        impl_after_imgui_begin_popup();

        // try to show popup
        if (not opened) {
            // popup not showing
            pimpl.is_open_ = false;
            return false;
        }
    }

    pimpl.is_open_ = true;
    return true;
}

void osc::Popup::end_popup()
{
    PopupPrivate& pimpl = private_data();
    ui::end_popup();
    pimpl.just_opened_ = false;
}

bool osc::Popup::is_popup_opened_this_frame() const
{
    return private_data().is_popup_opened_this_frame();
}
void osc::Popup::request_close()
{
    private_data().request_close();
}
bool osc::Popup::is_modal() const
{
    const PopupPrivate& pimpl = private_data();
    return pimpl.is_modal_;
}
void osc::Popup::set_modal(bool v)
{
    private_data().set_modal(v);
}
void osc::Popup::set_rect(const Rect& rect)
{
    PopupPrivate& pimpl = private_data();
    pimpl.maybe_position_ = rect.ypd_top_left();
    pimpl.dimensions_ = rect.dimensions();
}
void osc::Popup::set_dimensions(Vec2 d)
{
    PopupPrivate& pimpl = private_data();
    pimpl.dimensions_ = d;
}
void osc::Popup::set_position(std::optional<Vec2> p)
{
    PopupPrivate& pimpl = private_data();
    pimpl.maybe_position_ = p;
}

void osc::Popup::impl_on_draw()
{
    PopupPrivate& pimpl = private_data();
    if (pimpl.should_close_) {
        impl_on_close();
        ui::close_current_popup();
        pimpl.should_close_ = false;
        pimpl.should_open_ = false;
        pimpl.just_opened_ = false;
        return;
    }

    impl_draw_content();
}
