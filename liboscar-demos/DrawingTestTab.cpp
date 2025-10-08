#include "DrawingTestTab.h"

#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Platform/App.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/Assertions.h>

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
        RenderTexture tex{{
            .pixel_dimensions = Vector2i{static_cast<int>(App::get().main_window_device_pixel_ratio()*256.0f)},
            .device_pixel_ratio = App::get().main_window_device_pixel_ratio(),
        }};
        OSC_ASSERT(tex.dimensions() == Vector2{256.0f});

        ui::begin_panel("p");
        ui::DrawList dl;
        dl.push_clip_rect(Rect::from_corners(Vector2{}, tex.dimensions()));
        dl.add_circle({.origin = {}, .radius = 50.0f}, Color::red());
        dl.add_circle_filled({.origin = Vector2{128.0f}, .radius = 64.0f}, Color::purple());
        dl.add_rect_filled(Rect::from_corners(Vector2{128.0f}, Vector2{200.0f}), Color::blue(), 3.0f);
        dl.render_to(tex);
        dl.pop_clip_rect();
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
