#include "LOGLTexturingTab.hpp"

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
#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <cstdint>
#include <span>
#include <utility>

using osc::App;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::ImageLoadingFlags;
using osc::Mat4;
using osc::Material;
using osc::Mesh;
using osc::Shader;
using osc::Texture2D;
using osc::Vec2;
using osc::Vec3;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Texturing";

    Mesh GenerateTexturedQuadMesh()
    {
        Mesh quad = osc::GenTexturedQuad();

        // transform default quad verts to match LearnOpenGL
        quad.transformVerts([](Vec3& v) { v *= 0.5f; });

        // transform default quad texture coordinates to exercise wrap modes
        quad.transformTexCoords([](Vec2& coord) { coord *= 2.0f; });

        return quad;
    }

    Material LoadTexturedMaterial()
    {
        Material rv
        {
            Shader
            {
                App::slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.vert"),
                App::slurp("oscar_learnopengl/shaders/GettingStarted/Texturing.frag"),
            },
        };

        // set uTexture1
        {
            Texture2D container = osc::LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlags::FlipVertically
            );
            container.setWrapMode(osc::TextureWrapMode::Clamp);

            rv.setTexture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            Texture2D face = osc::LoadTexture2DFromImage(
                App::resource("oscar_learnopengl/textures/awesomeface.png"),
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
        rv.setViewMatrixOverride(osc::Identity<Mat4>());
        rv.setProjectionMatrixOverride(osc::Identity<Mat4>());
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
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

osc::UID osc::LOGLTexturingTab::implGetID() const
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
