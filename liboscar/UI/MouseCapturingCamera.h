#pragma once

#include <liboscar/Graphics/Camera.h>
#include <liboscar/Maths/EulerAngles.h>

namespace osc { class Event; }

namespace osc
{
    class MouseCapturingCamera final : public Camera {
    public:
        void on_mount();
        void on_unmount();
        bool on_event(Event&);
        void on_draw();

        bool is_capturing_mouse() const { return mouse_captured_; }

        const EulerAngles& eulers() const { return camera_eulers_; }
        EulerAngles& eulers() { return camera_eulers_; }

    private:
        void grab_mouse(bool);

        bool mouse_captured_ = false;
        EulerAngles camera_eulers_ = {};
    };
}
