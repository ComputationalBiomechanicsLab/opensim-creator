#include "ImGuiDemoTab.hpp"

#include <imgui.h>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/ImGui";
}

class osc::ImGuiDemoTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnDraw() final
    {
        ImGui::ShowDemoWindow();
    }
};


// public API

osc::CStringView osc::ImGuiDemoTab::id()
{
    return c_TabStringID;
}

osc::ImGuiDemoTab::ImGuiDemoTab(ParentPtr<TabHost> const&) :
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
