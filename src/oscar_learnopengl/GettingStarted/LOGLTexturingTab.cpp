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
        quad.transformTexCoords([](Vec2 coord) { return 2.0f * coord; });

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
            container.setWrapMode(TextureWrapMode::Clamp);

            rv.setTexture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            Texture2D const face = load_texture2D_from_image(
                rl.open("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );

            rv.setTexture("uTexture2", face);
        }

        return rv;
    }

    Camera CreateIdentityCamera()
    {
        Camera rv;
        rv.setViewMatrixOverride(identity<Mat4>());
        rv.setProjectionMatrixOverride(identity<Mat4>());
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnDraw() final
    {
        graphics::draw(m_Mesh, identity<Transform>(), m_Material, m_Camera);

        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
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

UID osc::LOGLTexturingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLTexturingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLTexturingTab::implOnDraw()
{
    m_Impl->onDraw();
}
