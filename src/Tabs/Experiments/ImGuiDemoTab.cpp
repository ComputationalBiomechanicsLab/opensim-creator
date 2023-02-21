#include "ImGuiDemoTab.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <string>
#include <utility>

class osc::ImGuiDemoTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_HAT_WIZARD " ImGuiDemoTab";
    }

    void onDraw()
    {
        ImGui::ShowDemoWindow();
    }

private:
    UID m_TabID;
};


// public API (PIMPL)

osc::CStringView osc::ImGuiDemoTab::id() noexcept
{
    return "Demos/ImGui";
}

osc::ImGuiDemoTab::ImGuiDemoTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ImGuiDemoTab::ImGuiDemoTab(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab& osc::ImGuiDemoTab::operator=(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab::~ImGuiDemoTab() noexcept = default;

osc::UID osc::ImGuiDemoTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ImGuiDemoTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ImGuiDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
