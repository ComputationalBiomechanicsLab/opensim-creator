#include "LOGLTexturingTab.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <cstdint>
#include <span>
#include <utility>

using osc::Mat4;
using osc::Vec2;
using osc::Vec3;

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Texturing";

    osc::Mesh GenerateTexturedQuadMesh()
    {
        osc::Mesh quad = osc::GenTexturedQuad();

        // transform default quad verts to match LearnOpenGL
        quad.transformVerts([](std::span<Vec3> vs)
        {
            for (Vec3& v : vs)
            {
                v *= 0.5f;
            }
        });

        // transform default quad texture coordinates to exercise wrap modes
        quad.transformTexCoords([](std::span<Vec2> coords)
        {
            for (Vec2& coord : coords)
            {
                coord *= 2.0f;
            }
        });

        return quad;
    }

    osc::Material LoadTexturedMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.vert"),
                osc::App::slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.frag"),
            },
        };

        // set uTexture1
        {
            osc::Texture2D container = osc::LoadTexture2DFromImage(
                osc::App::resource("oscar_learnopengl/textures/container.jpg"),
                osc::ColorSpace::sRGB,
                osc::ImageLoadingFlags::FlipVertically
            );
            container.setWrapMode(osc::TextureWrapMode::Clamp);

            rv.setTexture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            osc::Texture2D face = osc::LoadTexture2DFromImage(
                osc::App::resource("oscar_learnopengl/textures/awesomeface.png"),
                osc::ColorSpace::sRGB,
                osc::ImageLoadingFlags::FlipVertically
            );

            rv.setTexture("uTexture2", face);
        }

        return rv;
    }

    osc::Camera CreateIdentityCamera()
    {
        osc::Camera rv;
        rv.setViewMatrixOverride(Mat4{1.0f});
        rv.setProjectionMatrixOverride(Mat4{1.0f});
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnDraw() final
    {
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    Material m_Material = LoadTexturedMaterial();
    Mesh m_Mesh = GenerateTexturedQuadMesh();
    Camera m_Camera = CreateIdentityCamera();
};


// public API

osc::CStringView osc::LOGLTexturingTab::id()
{
    return c_TabStringID;
}

osc::LOGLTexturingTab::LOGLTexturingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLTexturingTab::LOGLTexturingTab(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab& osc::LOGLTexturingTab::operator=(LOGLTexturingTab&&) noexcept = default;
osc::LOGLTexturingTab::~LOGLTexturingTab() noexcept = default;

osc::UID osc::LOGLTexturingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLTexturingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLTexturingTab::implOnDraw()
{
    m_Impl->onDraw();
}
