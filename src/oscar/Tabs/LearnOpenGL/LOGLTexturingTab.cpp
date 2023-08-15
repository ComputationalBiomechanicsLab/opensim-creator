#include "LOGLTexturingTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/Tab.hpp"
#include "oscar/Tabs/TabRegistry.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Texturing";

    osc::Mesh GenerateTexturedQuadMesh()
    {
        osc::Mesh quad = osc::GenTexturedQuad();

        // transform default quad verts to match LearnOpenGL
        quad.transformVerts([](nonstd::span<glm::vec3> vs)
        {
            for (glm::vec3& v : vs)
            {
                v *= 0.5f;
            }
        });

        // transform default quad texture coordinates to exercise wrap modes
        quad.transformTexCoords([](nonstd::span<glm::vec2> coords)
        {
            for (glm::vec2& coord : coords)
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
                osc::App::slurp("shaders/ExperimentTexturing.vert"),
                osc::App::slurp("shaders/ExperimentTexturing.frag"),
            },
        };

        // set uTexture1
        {
            osc::Texture2D container = osc::LoadTexture2DFromImage(
                osc::App::resource("textures/container.jpg"),
                osc::ColorSpace::sRGB,
                osc::ImageLoadingFlags::FlipVertically
            );
            container.setWrapMode(osc::TextureWrapMode::Clamp);

            rv.setTexture("uTexture1", std::move(container));
        }

        // set uTexture2
        {
            osc::Texture2D face = osc::LoadTexture2DFromImage(
                osc::App::resource("textures/awesomeface.png"),
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
        rv.setViewMatrixOverride(glm::mat4{1.0f});
        rv.setProjectionMatrixOverride(glm::mat4{1.0f});
        return rv;
    }
}

class osc::LOGLTexturingTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onDraw()
    {
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

private:
    UID m_TabID;

    Material m_Material = LoadTexturedMaterial();
    Mesh m_Mesh = GenerateTexturedQuadMesh();
    Camera m_Camera = CreateIdentityCamera();
};


// public API (PIMPL)

osc::CStringView osc::LOGLTexturingTab::id() noexcept
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
