#include "LOGLHelloTriangleTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/HelloTriangle";

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

class osc::LOGLHelloTriangleTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_draw() final
    {
        graphics::draw(triangle_mesh_, identity<Transform>(), material_, camera_);

        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screen_rect());
        camera_.render_to_screen();
    }

    ResourceLoader loader_ = App::resource_loader();
    Material material_ = create_triangle_material(loader_);
    Mesh triangle_mesh_ = generate_triangle_mesh();
    Camera camera_ = create_scene_camera();
};


CStringView osc::LOGLHelloTriangleTab::id()
{
    return c_tab_string_id;
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab& osc::LOGLHelloTriangleTab::operator=(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab::~LOGLHelloTriangleTab() noexcept = default;

UID osc::LOGLHelloTriangleTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLHelloTriangleTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLHelloTriangleTab::impl_on_draw()
{
    impl_->on_draw();
}
