#pragma once

#include <SDL_events.h>
#include <imgui.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>

namespace osc
{
    class MouseCapturingCamera final : public Camera {
    public:
        void onMount()
        {
            m_IsMouseCaptured = true;
            App::upd().setShowCursor(false);
        }

        void onUnmount()
        {
            m_IsMouseCaptured = false;
            App::upd().setShowCursor(true);
        }

        bool onEvent(SDL_Event const& e)
        {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                m_IsMouseCaptured = false;
                return true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect()) {
                m_IsMouseCaptured = true;
                return true;
            }

            return false;
        }

        void onDraw()
        {
            // handle mouse capturing
            if (m_IsMouseCaptured) {
                UpdateEulerCameraFromImGuiUserInput(*this, m_CameraEulers);
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);
                App::upd().setShowCursor(false);
            }
            else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
                App::upd().setShowCursor(true);
            }
        }

        bool isCapturingMouse() const
        {
            return m_IsMouseCaptured;
        }

        Eulers const& eulers() const
        {
            return m_CameraEulers;
        }

        Eulers& eulers()
        {
            return m_CameraEulers;
        }

    private:
        bool m_IsMouseCaptured = false;
        Eulers m_CameraEulers = {};
    };
}
