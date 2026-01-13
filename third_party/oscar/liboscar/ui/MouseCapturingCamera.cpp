#include "MouseCapturingCamera.h"

#include <liboscar/platform/App.h>
#include <liboscar/platform/Cursor.h>
#include <liboscar/platform/CursorShape.h>
#include <liboscar/platform/events/Event.h>
#include <liboscar/platform/events/KeyEvent.h>
#include <liboscar/ui/oscimgui.h>

#include <utility>

void osc::MouseCapturingCamera::on_mount()
{
    grab_mouse(true);
}

void osc::MouseCapturingCamera::on_unmount()
{
    grab_mouse(false);
}

bool osc::MouseCapturingCamera::on_event(Event& e)
{
    if (e.type() == EventType::KeyUp and dynamic_cast<const KeyEvent&>(e).combination() == Key::Escape) {
        grab_mouse(false);
    }
    else if (e.type() == EventType::MouseButtonDown and ui::is_mouse_in_main_window_workspace()) {
        grab_mouse(true);
    }
    return false;
}

void osc::MouseCapturingCamera::on_draw()
{
    if (mouse_captured_) {
        ui::update_camera_from_all_inputs(*this, camera_eulers_);
    }
}

void osc::MouseCapturingCamera::grab_mouse(bool v)
{
    if (v) {
        if (not std::exchange(mouse_captured_, true)) {
            App::upd().push_cursor_override(Cursor{CursorShape::Hidden});
            App::upd().enable_main_window_grab();
        }
    }
    else {
        if (std::exchange(mouse_captured_, false)) {
            App::upd().disable_main_window_grab();
            App::upd().pop_cursor_override();
        }
    }
}
