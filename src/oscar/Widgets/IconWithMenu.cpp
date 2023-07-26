#include "IconWithMenu.hpp"

#include "oscar/Graphics/Icon.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Widgets/IconWithoutMenu.hpp"

#include <imgui.h>

#include <functional>
#include <string>
#include <utility>

osc::IconWithMenu::IconWithMenu(
    osc::Icon icon,
    osc::CStringView title,
    osc::CStringView description,
    std::function<void()> contentRenderer) :

    m_IconWithoutMenu{std::move(icon), title, description},
    m_ContextMenuID{"##" + m_IconWithoutMenu.getIconID()},
    m_ContentRenderer{std::move(contentRenderer)}
{
}

void osc::IconWithMenu::onDraw()
{
    if (m_IconWithoutMenu.onDraw())
    {
        ImGui::OpenPopup(m_ContextMenuID.c_str());
    }

    if (ImGui::BeginPopup(m_ContextMenuID.c_str(),ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::TextDisabled("%s", m_IconWithoutMenu.getTitle().c_str());
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});
        m_ContentRenderer();
        ImGui::EndPopup();
    }
}
