#include "ImGuiDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc;

class osc::ImGuiDemoTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Demos/ImGui"; }

    Impl() : TabPrivate{static_label()} {}

    void on_draw()
    {
        ui::show_demo_panel();
    }
};


CStringView osc::ImGuiDemoTab::id()
{
    return Impl::static_label();
}

osc::ImGuiDemoTab::ImGuiDemoTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}

void osc::ImGuiDemoTab::impl_on_draw()
{
    private_data().on_draw();
}
