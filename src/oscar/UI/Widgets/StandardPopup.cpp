#include "StandardPopup.h"

#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/oscimgui.h>

#include <optional>
#include <string_view>

osc::StandardPopup::StandardPopup(std::string_view popupName) :
    StandardPopup{popupName, {512.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize}
{
}

osc::StandardPopup::StandardPopup(
    std::string_view popupName,
    Vec2 dimensions,
    ImGuiWindowFlags popupFlags) :

    m_PopupName{popupName},
    m_Dimensions{dimensions},
    m_MaybePosition{std::nullopt},
    m_PopupFlags{popupFlags},
    m_ShouldOpen{false},
    m_ShouldClose{false},
    m_JustOpened{false},
    m_IsOpen{false},
    m_IsModal{true}
{
}

bool osc::StandardPopup::implIsOpen() const
{
    return m_ShouldOpen || m_IsOpen;
}

void osc::StandardPopup::implOpen()
{
    m_ShouldOpen = true;
    m_ShouldClose = false;
}

void osc::StandardPopup::implClose()
{
    m_ShouldClose = true;
    m_ShouldOpen = false;
}

bool osc::StandardPopup::implBeginPopup()
{
    if (m_ShouldOpen)
    {
        ImGui::OpenPopup(m_PopupName.c_str());
        m_ShouldOpen = false;
        m_ShouldClose = false;
        m_JustOpened = true;
    }

    if (m_IsModal)
    {
        // if specified, set the position of the modal upon appearing
        //
        // else, position the modal in the center of the viewport
        if (m_MaybePosition)
        {
            ImGui::SetNextWindowPos(
                static_cast<Vec2>(*m_MaybePosition),
                ImGuiCond_Appearing
            );
        }
        else
        {
            ImGui::SetNextWindowPos(
                ImGui::GetMainViewport()->GetCenter(),
                ImGuiCond_Appearing,
                ImVec2{0.5f, 0.5f}
            );
        }

        // if the modal doesn't auto-resize each frame, then set the size of
        // modal only upon appearing
        //
        // else, set the position every frame, because the __nonzero__ dimensions
        // will stretch out the modal accordingly
        if (!(m_PopupFlags & ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SetNextWindowSize(
                Vec2{m_Dimensions},
                ImGuiCond_Appearing
            );
        }
        else
        {
            ImGui::SetNextWindowSize(
                Vec2{m_Dimensions}
            );
        }

        // try to begin the modal window
        implBeforeImguiBeginPopup();
        bool const opened = ImGui::BeginPopupModal(m_PopupName.c_str(), nullptr, m_PopupFlags);
        implAfterImguiBeginPopup();

        if (!opened)
        {
            // modal not showing
            m_IsOpen = false;
            return false;
        }
    }
    else
    {
        // if specified, set the position of the modal upon appearing
        //
        // else, do nothing - the popup's position will be determined
        // by other means (unlike a modal, which usually takes control
        // of the screen and, therefore, should proabably be centered
        // in it)
        if (m_MaybePosition)
        {
            ImGui::SetNextWindowPos(
                static_cast<Vec2>(*m_MaybePosition),
                ImGuiCond_Appearing
            );
        }

        // try to begin the popup window
        implBeforeImguiBeginPopup();
        bool const opened = ImGui::BeginPopup(m_PopupName.c_str(), m_PopupFlags);
        implAfterImguiBeginPopup();

        // try to show popup
        if (!opened)
        {
            // popup not showing
            m_IsOpen = false;
            return false;
        }
    }

    m_IsOpen = true;
    return true;
}

void osc::StandardPopup::implOnDraw()
{
    if (m_ShouldClose)
    {
        implOnClose();
        ImGui::CloseCurrentPopup();
        m_ShouldClose = false;
        m_ShouldOpen = false;
        m_JustOpened = false;
        return;
    }

    implDrawContent();
}

void osc::StandardPopup::implEndPopup()
{
    ImGui::EndPopup();
    m_JustOpened = false;
}

bool osc::StandardPopup::isPopupOpenedThisFrame() const
{
    return m_JustOpened;
}

void osc::StandardPopup::requestClose()
{
    m_ShouldClose = true;
    m_ShouldOpen = false;
}

bool osc::StandardPopup::isModal() const
{
    return m_IsModal;
}

void osc::StandardPopup::setModal(bool v)
{
    m_IsModal = v;
}

void osc::StandardPopup::setRect(Rect const& rect)
{
    m_MaybePosition = rect.p1;
    m_Dimensions = dimensions(rect);
}

void osc::StandardPopup::setDimensions(Vec2 d)
{
    m_Dimensions = d;
}

void osc::StandardPopup::setPosition(std::optional<Vec2> p)
{
    m_MaybePosition = p;
}

