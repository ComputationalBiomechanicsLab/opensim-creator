#include "ImGuiDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc;

class osc::ImGuiDemoTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/ImGui"; }

    explicit Impl(ImGuiDemoTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {}

    void on_draw()
    {
        ui::show_demo_panel();
    }
};


CStringView osc::ImGuiDemoTab::id() { return Impl::static_label(); }

osc::ImGuiDemoTab::ImGuiDemoTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ImGuiDemoTab::impl_on_draw() { private_data().on_draw(); }
