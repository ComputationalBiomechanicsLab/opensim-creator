#include "logl_hello_triangle_tab.h"

#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/render_pass_config.h>
#include <liboscar/graphics/render_queue.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <memory>

using namespace osc;

namespace
{
    Mesh generate_triangle_mesh()
    {
        Mesh mesh;
        mesh.set_vertices({
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        });
        mesh.set_colors({
            Color::red(),
            Color::green(),
            Color::blue(),
        });
        mesh.set_indices({0, 1, 2});
        return mesh;
    }

    Camera create_scene_camera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Matrix4x4>());
        rv.set_projection_matrix_override(identity<Matrix4x4>());
        return rv;
    }

    Material create_triangle_material(ResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/GettingStarted/HelloTriangle.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/GettingStarted/HelloTriangle.frag"),
        }};
    }
}

class osc::LOGLHelloTriangleTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/GettingStarted/HelloTriangle"; }

    explicit Impl(LOGLHelloTriangleTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    void on_draw()
    {
        render_queue_.emplace(triangle_mesh_, identity<Transform>(), material_);
        graphics::render_to_main_window(render_queue_, camera_, {
            .viewport_rect = ui::get_main_window_workspace_screen_space_rect(),
        });
        render_queue_.clear();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Material material_ = create_triangle_material(loader_);
    Mesh triangle_mesh_ = generate_triangle_mesh();
    Camera camera_ = create_scene_camera();
    RenderQueue render_queue_;
};


CStringView osc::LOGLHelloTriangleTab::id() { return Impl::static_label(); }

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLHelloTriangleTab::impl_on_draw() { private_data().on_draw(); }
