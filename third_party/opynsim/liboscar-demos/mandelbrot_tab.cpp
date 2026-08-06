#include "mandelbrot_tab.h"

#include <liboscar/graphics/geometries/plane_geometry.h>
#include <liboscar/graphics/camera_v2.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/render_queue.h>
#include <liboscar/graphics/render_pass_config.h>
#include <liboscar/graphics/shader.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/platform/events/event_type.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/events/mouse_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <limits>
#include <memory>

using namespace osc;

namespace
{
    CameraV2 create_identity_camera()
    {
        CameraV2 rv;
        rv.set_view_matrix_override(identity<Matrix4x4>());
        rv.set_projection_matrix_override(identity<Matrix4x4>());
        return rv;
    }
}

class osc::MandelbrotTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/Mandelbrot"; }

    explicit Impl(MandelbrotTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    bool on_event(Event& ev)
    {
        if (ev.type() == EventType::KeyUp) {
            return on_keyup(dynamic_cast<const KeyEvent&>(ev));
        }
        return false;
    }

    void on_draw()
    {
        material_.set("uRescale", Vector2{1.0f, 1.0f});
        material_.set("uOffset", Vector2{});
        material_.set("uNumIterations", num_iterations_);

        render_queue_.clear();
        render_queue_.emplace(quad_mesh_, identity<Transform>(), material_);
        graphics::render_to_main_window(render_queue_, camera_, {
            .viewport_rect = ui::get_main_window_workspace_screen_space_rect(),
        });
    }

private:
    bool on_keyup(const KeyEvent& e)
    {
        if (e.combination() == Key::UpArrow and num_iterations_ < std::numeric_limits<decltype(num_iterations_)>::max()) {
            num_iterations_ *= 2;
            return true;
        }
        if (e.combination() == Key::DownArrow and num_iterations_ > 1) {
            num_iterations_ /= 2;
            return true;
        }
        return false;
    }

    ResourceLoader loader_ = App::resource_loader();
    int num_iterations_ = 16;
    Mesh quad_mesh_ = PlaneGeometry{{.dimensions = Vector2{2.0f}}};
    Material material_{Shader{
        loader_.slurp("oscar_demos/shaders/Mandelbrot.vert"),
        loader_.slurp("oscar_demos/shaders/Mandelbrot.frag"),
    }};
    CameraV2 camera_ = create_identity_camera();
    RenderQueue render_queue_;
};


CStringView osc::MandelbrotTab::id() { return Impl::static_label(); }

osc::MandelbrotTab::MandelbrotTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
bool osc::MandelbrotTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::MandelbrotTab::impl_on_draw() { private_data().on_draw(); }
