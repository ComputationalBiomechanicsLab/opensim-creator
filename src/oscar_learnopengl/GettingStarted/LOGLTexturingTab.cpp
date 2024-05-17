#include "LOGLTexturingTab.h"

#include <oscar/oscar.h>

#include <memory>
#include <utility>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/Texturing";

    Mesh generate_textured_quad_mesh()
    {
        Mesh quad = PlaneGeometry{1.0f, 1.0f, 1, 1};

        // transform default quad texture coordinates to exercise wrap modes
        quad.transform_tex_coords([](Vec2 coord) { return 2.0f * coord; });

        return quad;
    }

    Material load_textured_material(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.vert"),
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.frag"),
        }};

        // set uTexture1
        {
            Texture2D container = load_texture2D_from_image(
                loader.open("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );
            container.set_wrap_mode(TextureWrapMode::Clamp);

            rv.set_texture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            const Texture2D face = load_texture2D_from_image(
                loader.open("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );

            rv.set_texture("uTexture2", face);
        }

        return rv;
    }

    Camera create_identity_camera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Mat4>());
        rv.set_projection_matrix_override(identity<Mat4>());
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_draw() final
    {
        graphics::draw(mesh_, identity<Transform>(), material_, camera_);

        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.render_to_screen();
    }

    ResourceLoader loader_ = App::resource_loader();
    Material material_ = load_textured_material(loader_);
    Mesh mesh_ = generate_textured_quad_mesh();
    Camera camera_ = create_identity_camera();
};


CStringView osc::LOGLTexturingTab::id()
{
    return c_tab_string_id;
}

osc::LOGLTexturingTab::LOGLTexturingTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLTexturingTab::LOGLTexturingTab(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab& osc::LOGLTexturingTab::operator=(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab::~LOGLTexturingTab() noexcept = default;

UID osc::LOGLTexturingTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLTexturingTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLTexturingTab::impl_on_draw()
{
    impl_->on_draw();
}
