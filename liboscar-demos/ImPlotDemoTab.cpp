#include "ImPlotDemoTab.h"

#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

class osc::ImPlotDemoTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/ImPlot"; }

    explicit Impl(ImPlotDemoTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {}

    void on_draw()
    {
        ui::plot::show_demo_panel();
    }
};


CStringView osc::ImPlotDemoTab::id() { return Impl::static_label(); }

osc::ImPlotDemoTab::ImPlotDemoTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ImPlotDemoTab::impl_on_draw() { private_data().on_draw(); }
