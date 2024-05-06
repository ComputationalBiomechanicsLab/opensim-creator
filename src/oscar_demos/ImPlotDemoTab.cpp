#include "ImPlotDemoTab.h"

#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/ImPlot";
}

class osc::ImPlotDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        // ImPlot::CreateContext();  // presumed to already done by the screen
    }

private:
    void impl_on_draw() final
    {
        ImPlot::ShowDemoWindow();
    }
};


// public API

CStringView osc::ImPlotDemoTab::id()
{
    return c_TabStringID;
}

osc::ImPlotDemoTab::ImPlotDemoTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::ImPlotDemoTab::ImPlotDemoTab(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab& osc::ImPlotDemoTab::operator=(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab::~ImPlotDemoTab() noexcept = default;

UID osc::ImPlotDemoTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::ImPlotDemoTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::ImPlotDemoTab::impl_on_draw()
{
    m_Impl->on_draw();
}
