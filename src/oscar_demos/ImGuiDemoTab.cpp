#include "ImGuiDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/ImGui";
}

class osc::ImGuiDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnDraw() final
    {
        ui::ShowDemoWindow();
    }
};


// public API

CStringView osc::ImGuiDemoTab::id()
{
    return c_TabStringID;
}

osc::ImGuiDemoTab::ImGuiDemoTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::ImGuiDemoTab::ImGuiDemoTab(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab& osc::ImGuiDemoTab::operator=(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab::~ImGuiDemoTab() noexcept = default;

UID osc::ImGuiDemoTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::ImGuiDemoTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ImGuiDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
