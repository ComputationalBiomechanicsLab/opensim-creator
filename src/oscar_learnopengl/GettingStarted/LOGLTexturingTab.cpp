#include "LOGLTexturingTab.h"

#include <oscar/oscar.h>

#include <memory>
#include <utility>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Texturing";

    Mesh GenTexturedQuadMesh()
    {
        Mesh quad = PlaneGeometry{1.0f, 1.0f, 1, 1};

        // transform default quad texture coordinates to exercise wrap modes
        quad.transform_tex_coords([](Vec2 coord) { return 2.0f * coord; });

        return quad;
    }

    Material LoadTexturedMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.vert"),
            rl.slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.frag"),
        }};

        // set uTexture1
        {
            Texture2D container = load_texture2D_from_image(
                rl.open("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );
            container.set_wrap_mode(TextureWrapMode::Clamp);

            rv.set_texture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            Texture2D const face = load_texture2D_from_image(
                rl.open("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );

            rv.set_texture("uTexture2", face);
        }

        return rv;
    }

    Camera CreateIdentityCamera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Mat4>());
        rv.set_projection_matrix_override(identity<Mat4>());
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void impl_on_draw() final
    {
        graphics::draw(m_Mesh, identity<Transform>(), m_Material, m_Camera);

        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.render_to_screen();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_Material = LoadTexturedMaterial(m_Loader);
    Mesh m_Mesh = GenTexturedQuadMesh();
    Camera m_Camera = CreateIdentityCamera();
};


// public API

CStringView osc::LOGLTexturingTab::id()
{
    return c_TabStringID;
}

osc::LOGLTexturingTab::LOGLTexturingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLTexturingTab::LOGLTexturingTab(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab& osc::LOGLTexturingTab::operator=(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab::~LOGLTexturingTab() noexcept = default;

UID osc::LOGLTexturingTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLTexturingTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLTexturingTab::impl_on_draw()
{
    m_Impl->on_draw();
}
