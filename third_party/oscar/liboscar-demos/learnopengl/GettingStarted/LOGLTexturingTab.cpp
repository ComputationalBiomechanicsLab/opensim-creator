#include "LOGLTexturingTab.h"

#include <liboscar/Formats/Image.h>
#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Geometries/PlaneGeometry.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/IResourceLoader.h>
#include <liboscar/Platform/ResourceLoader.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>

#include <memory>
#include <utility>

using namespace osc;

namespace
{
    Mesh generate_textured_quad_mesh()
    {
        Mesh quad = PlaneGeometry{};

        // transform default quad texture coordinates to exercise wrap modes
        quad.transform_tex_coords([](Vector2 uv) { return 2.0f * uv; });

        return quad;
    }

    Material load_textured_material(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/GettingStarted/Texturing.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/GettingStarted/Texturing.frag"),
        }};

        // set uTexture1
        {
            Texture2D container = Image::read_into_texture(
                loader.open("oscar_demos/learnopengl/textures/container.jpg"),
                ColorSpace::sRGB
            );
            container.set_wrap_mode(TextureWrapMode::Clamp);

            rv.set("uTexture1", container);
        }

        // set uTexture2
        {
            const Texture2D face = Image::read_into_texture(
                loader.open("oscar_demos/learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB
            );

            rv.set("uTexture2", face);
        }

        return rv;
    }

    Camera create_identity_camera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Matrix4x4>());
        rv.set_projection_matrix_override(identity<Matrix4x4>());
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/GettingStarted/Texturing"; }

    explicit Impl(LOGLTexturingTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    void on_draw()
    {
        graphics::draw(mesh_, identity<Transform>(), material_, camera_);

        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        camera_.render_to_main_window();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Material material_ = load_textured_material(loader_);
    Mesh mesh_ = generate_textured_quad_mesh();
    Camera camera_ = create_identity_camera();
};


CStringView osc::LOGLTexturingTab::id() { return Impl::static_label(); }

osc::LOGLTexturingTab::LOGLTexturingTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLTexturingTab::impl_on_draw() { private_data().on_draw(); }
