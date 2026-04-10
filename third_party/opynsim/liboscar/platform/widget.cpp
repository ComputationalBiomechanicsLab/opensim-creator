#include "widget.h"

#include <liboscar/platform/events/display_state_change_event.h>
#include <liboscar/platform/events/drop_file_event.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/event_type.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/events/mouse_event.h>
#include <liboscar/platform/events/mouse_wheel_event.h>
#include <liboscar/platform/events/quit_event.h>
#include <liboscar/platform/events/text_input_event.h>
#include <liboscar/platform/events/window_event.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/utilities/lifetimed_ptr.h>

using namespace osc;

osc::Widget::Widget(Widget* parent) :
    data_{std::make_unique<WidgetPrivate>(*this, parent)}
{}
osc::Widget::~Widget() noexcept = default;

Widget* osc::Widget::parent() { return data_->parent(); }
const Widget* osc::Widget::parent() const { return data_->parent(); }
void osc::Widget::set_parent(Widget* new_parent) { data_->set_parent(new_parent); }

LifetimedPtr<Widget> osc::Widget::weak_ref()
{
    return {private_data().lifetime_watcher(), this};
}

CStringView osc::Widget::name() const
{
    return private_data().name();
}

void osc::Widget::set_name(std::string_view name)
{
    private_data().set_name(name);
}

osc::Widget::Widget(std::unique_ptr<WidgetPrivate>&& data) :
    data_{std::move(data)}
{}

bool osc::Widget::impl_on_event(Event& e)
{
    switch (e.type()) {
    case EventType::DropFile:           return impl_on_drop_file_event(dynamic_cast<DropFileEvent&>(e));
    case EventType::KeyDown:            [[fallthrough]];
    case EventType::KeyUp:              return impl_on_key_event(dynamic_cast<KeyEvent&>(e));
    case EventType::TextInput:          return impl_on_text_input_event(dynamic_cast<TextInputEvent&>(e));
    case EventType::MouseButtonDown:    [[fallthrough]];
    case EventType::MouseButtonUp:      [[fallthrough]];
    case EventType::MouseMove:          return impl_on_mouse_event(dynamic_cast<MouseEvent&>(e));
    case EventType::MouseWheel:         return impl_on_mouse_wheel_event(dynamic_cast<MouseWheelEvent&>(e));
    case EventType::DisplayStateChange: return impl_on_display_state_change_event(dynamic_cast<DisplayStateChangeEvent&>(e));
    case EventType::Quit:               return impl_on_quit_event(dynamic_cast<QuitEvent&>(e));
    case EventType::Window:             return impl_on_window_event(dynamic_cast<WindowEvent&>(e));
    default:                            return false;
    }
}

bool osc::Widget::impl_on_display_state_change_event(DisplayStateChangeEvent&) { return false; }
bool osc::Widget::impl_on_drop_file_event(DropFileEvent&) { return false; }
bool osc::Widget::impl_on_key_event(KeyEvent&) { return false; }
bool osc::Widget::impl_on_mouse_event(MouseEvent&) { return false; }
bool osc::Widget::impl_on_mouse_wheel_event(MouseWheelEvent&) { return false; }
bool osc::Widget::impl_on_quit_event(QuitEvent&) { return false; }
bool osc::Widget::impl_on_text_input_event(TextInputEvent&) { return false; }
bool osc::Widget::impl_on_window_event(WindowEvent&) { return false; }

void osc::Widget::impl_on_mount() {}
void osc::Widget::impl_on_unmount() {}
void osc::Widget::impl_on_tick() {}
void osc::Widget::impl_on_draw() {}
