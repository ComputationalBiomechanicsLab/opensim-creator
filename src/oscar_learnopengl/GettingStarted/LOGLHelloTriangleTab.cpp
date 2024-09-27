#include "LOGLHelloTriangleTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc;

namespace
{
    Mesh generate_triangle_mesh()
    {
        Mesh m;
        m.set_vertices({
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        });
        m.set_colors({
            Color::red(),
            Color::green(),
            Color::blue(),
        });
        m.set_indices({0, 1, 2});
        return m;
    }

    Camera create_scene_camera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Mat4>());
        rv.set_projection_matrix_override(identity<Mat4>());
        return rv;
    }

    Material create_triangle_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.vert"),
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.frag"),
        }};
    }
}

class osc::LOGLHelloTriangleTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "LearnOpenGL/HelloTriangle"; }

    Impl() : TabPrivate{static_label()} {}

    void on_draw()
    {
        graphics::draw(triangle_mesh_, identity<Transform>(), material_, camera_);

        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.render_to_screen();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Material material_ = create_triangle_material(loader_);
    Mesh triangle_mesh_ = generate_triangle_mesh();
    Camera camera_ = create_scene_camera();
};


CStringView osc::LOGLHelloTriangleTab::id() { return Impl::static_label(); }

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}

void osc::LOGLHelloTriangleTab::impl_on_draw() { private_data().on_draw(); }
