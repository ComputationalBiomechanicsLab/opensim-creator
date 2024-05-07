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
    void impl_on_draw() final
    {
        ui::show_demo_panel();
    }
};


CStringView osc::ImGuiDemoTab::id()
{
    return c_TabStringID;
}

osc::ImGuiDemoTab::ImGuiDemoTab(const ParentPtr<ITabHost>&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::ImGuiDemoTab::ImGuiDemoTab(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab& osc::ImGuiDemoTab::operator=(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab::~ImGuiDemoTab() noexcept = default;

UID osc::ImGuiDemoTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::ImGuiDemoTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::ImGuiDemoTab::impl_on_draw()
{
    m_Impl->on_draw();
}
