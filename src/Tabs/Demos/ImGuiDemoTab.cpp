#include "ImGuiDemoTab.hpp"

#include "src/Utils/CStringView.hpp"

#include <imgui.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/ImGui";
}

class osc::ImGuiDemoTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
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
    return c_TabStringID;
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
