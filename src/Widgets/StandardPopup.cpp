#include "StandardPopup.hpp"

#include <imgui.h>

#include <string_view>
#include <utility>

osc::StandardPopup::StandardPopup(std::string_view popupName) :
    StandardPopup{std::move(popupName), 512.0f, 0.0f, ImGuiWindowFlags_AlwaysAutoResize}
{
}

osc::StandardPopup::StandardPopup(
    std::string_view popupName,
    float width,
    float height,
    int popupFlags) :

    m_PopupName{std::move(popupName)},
    m_Width{std::move(width)},
    m_Height{std::move(height)},
    m_PopupFlags{std::move(popupFlags)},
    m_ShouldOpen{false},
    m_ShouldClose{false},
    m_JustOpened{false},
    m_IsOpen{false}
{
}

bool osc::StandardPopup::isOpen() const
{
    return m_ShouldOpen || m_IsOpen;
}

void osc::StandardPopup::open()
{
    m_ShouldOpen = true;
    m_ShouldClose = false;
}

void osc::StandardPopup::close()
{
    m_ShouldClose = true;
    m_ShouldOpen = false;
}

void osc::StandardPopup::draw()
{
    if (m_ShouldOpen)
    {
        ImGui::OpenPopup(m_PopupName.c_str());
        m_ShouldOpen = false;
        m_ShouldClose = false;
        m_JustOpened = true;
    }

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
        ImGui::SetNextWindowSize({m_Width, m_Height});
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(m_PopupName.c_str(), nullptr, m_PopupFlags))
    {
        // modal not showing
        m_IsOpen = false;
        return;
    }

    m_IsOpen = true;

    if (m_ShouldClose)
    {
        implOnClose();
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        m_ShouldClose = false;
        m_ShouldOpen = false;
        m_JustOpened = false;
        return;
    }

    implDraw();

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
