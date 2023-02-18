#include "StandardPopup.hpp"

#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

osc::StandardPopup::StandardPopup(std::string_view popupName) :
    StandardPopup{std::move(popupName), {512.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize}
{
}

osc::StandardPopup::StandardPopup(
    std::string_view popupName,
    glm::vec2 dimensions,
    ImGuiWindowFlags popupFlags) :

    m_PopupName{std::move(popupName)},
    m_Dimensions{dimensions},
    m_MaybePosition{std::nullopt},
    m_PopupFlags{std::move(popupFlags)},
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
        // center the modal if no position is specified (otherwise: use the position)
        {
            if (m_MaybePosition)
            {
                ImGui::SetNextWindowPos(static_cast<glm::vec2>(*m_MaybePosition));
            }
            else
            {
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
            }

            // set size (modals only?)
            ImGui::SetNextWindowSize(glm::vec2{m_Dimensions});
        }

        // try to show modal
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
        if (m_MaybePosition)
        {
            ImGui::SetNextWindowPos(static_cast<glm::vec2>(*m_MaybePosition));
        }

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

void osc::StandardPopup::implDrawPopupContent()
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
    m_IsModal = std::move(v);
}

void osc::StandardPopup::setRect(osc::Rect const& rect)
{
    m_MaybePosition = rect.p1;
    m_Dimensions = osc::Dimensions(rect);
}

void osc::StandardPopup::setDimensions(glm::vec2 d)
{
    m_Dimensions = d;
}

void osc::StandardPopup::setPosition(std::optional<glm::vec2> p)
{
    m_MaybePosition = p;
}

