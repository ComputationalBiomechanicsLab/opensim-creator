#include "ImPlotDemoTab.h"

#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "Demos/ImPlot";
}

class osc::ImPlotDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {
        // ui::context::init()  // presumed to already done by the screen
    }

private:
    void impl_on_draw() final
    {
        ui::plot::show_demo_panel();
    }
};


CStringView osc::ImPlotDemoTab::id()
{
    return c_tab_string_id;
}

osc::ImPlotDemoTab::ImPlotDemoTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}

osc::ImPlotDemoTab::ImPlotDemoTab(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab& osc::ImPlotDemoTab::operator=(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab::~ImPlotDemoTab() noexcept = default;

UID osc::ImPlotDemoTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::ImPlotDemoTab::impl_get_name() const
{
    return impl_->name();
}

void osc::ImPlotDemoTab::impl_on_draw()
{
    impl_->on_draw();
}
