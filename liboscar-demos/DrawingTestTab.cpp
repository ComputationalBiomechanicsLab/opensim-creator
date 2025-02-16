#include "DrawingTestTab.h"

#include <liboscar/oscar.h>

#include <memory>

using namespace osc;

class osc::DrawingTestTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/DrawingTest"; }

    explicit Impl(
        DrawingTestTab& owner,
        Widget* parent) :

        TabPrivate{owner, parent, static_label()}
    {}

    bool on_event(Event&) { return false; }
    void on_tick() {}
    void on_draw()
    {
        const Rect r = ui::get_main_viewport_workspace_screenspace_rect();

        RenderTexture tex{{
            .dimensions = {256.0f, 256.0f},
            .device_pixel_ratio = App::get().main_window_device_pixel_ratio(),
        }};

        ui::begin_panel("p");
        ui::DrawList dl;
        dl.add_circle({.origin = {}, .radius = 50.0f}, Color::red());
        dl.add_circle_filled({.origin = Vec2{125.0f}, .radius = 100.0f}, Color::red());
        dl.add_rect_filled({{0.0f, 0.0f}, {100.0f, 100.0f}}, Color::blue(), 3.0f);
        dl.render_to(tex);
        ui::draw_image(tex);
        ui::end_panel();
    }
};

CStringView osc::DrawingTestTab::id() { return Impl::static_label(); }
osc::DrawingTestTab::DrawingTestTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
bool osc::DrawingTestTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::DrawingTestTab::impl_on_tick() { private_data().on_tick();  }
void osc::DrawingTestTab::impl_on_draw() { private_data().on_draw(); }
