#include "MouseCapturingCamera.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/Event.h>
#include <oscar/UI/oscimgui.h>

void osc::MouseCapturingCamera::on_mount()
{
    mouse_captured_ = true;
    App::upd().set_show_cursor(false);
}

void osc::MouseCapturingCamera::on_unmount()
{
    mouse_captured_ = false;
    App::upd().set_show_cursor(true);
}

bool osc::MouseCapturingCamera::on_event(Event& e)
{
    if (e.type() == EventType::KeyUp) {
        if (dynamic_cast<const KeyEvent&>(e).matches(Key::Escape)) {
            mouse_captured_ = false;
            return true;
        }
    }
    else if (e.type() == EventType::MouseButtonDown) {
        if (ui::is_mouse_in_main_viewport_workspace()) {
            mouse_captured_ = true;
            return true;
        }
    }
    return false;
}

void osc::MouseCapturingCamera::on_draw()
{
    // handle mouse capturing
    if (mouse_captured_) {
        ui::update_camera_from_all_inputs(*this, camera_eulers_);
        ui::hide_mouse_cursor();
        App::upd().set_show_cursor(false);
    }
    else {
        ui::show_mouse_cursor();
        App::upd().set_show_cursor(true);
    }
}
