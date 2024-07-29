#pragma once

#include <SDL_events.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Platform/App.h>

namespace osc
{
    class MouseCapturingCamera final : public Camera {
    public:
        void on_mount()
        {
            mouse_captured_ = true;
            App::upd().set_show_cursor(false);
        }

        void on_unmount()
        {
            mouse_captured_ = false;
            App::upd().set_show_cursor(true);
        }

        bool on_event(const SDL_Event& e)
        {
            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
                mouse_captured_ = false;
                return true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN and ui::is_mouse_in_main_viewport_workspace()) {
                mouse_captured_ = true;
                return true;
            }

            return false;
        }

        void on_draw()
        {
            // handle mouse capturing
            if (mouse_captured_) {
                ui::update_camera_from_all_inputs(*this, camera_eulers_);
                ui::set_mouse_cursor(ImGuiMouseCursor_None);
                App::upd().set_show_cursor(false);
            }
            else {
                ui::set_mouse_cursor(ImGuiMouseCursor_Arrow);
                App::upd().set_show_cursor(true);
            }
        }

        bool is_capturing_mouse() const { return mouse_captured_; }

        const EulerAngles& eulers() const { return camera_eulers_; }
        EulerAngles& eulers() { return camera_eulers_; }

    private:
        bool mouse_captured_ = false;
        EulerAngles camera_eulers_ = {};
    };
}
