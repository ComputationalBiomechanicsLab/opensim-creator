#pragma once

#include <oscar/Graphics/Camera.hpp>
#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>

#include <imgui.h>
#include <SDL_events.h>

#include <optional>
#include <utility>

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
            else if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect()) {
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
