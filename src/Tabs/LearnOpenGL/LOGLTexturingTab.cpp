#include "LOGLTexturingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/ColorSpace.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Tabs/TabRegistry.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Texturing";

    osc::Mesh GenerateMesh()
    {
        osc::Mesh quad = osc::GenTexturedQuad();

        quad.transformVerts([](nonstd::span<glm::vec3> vs)
            {
                for (glm::vec3& v : vs)
                {
                    v *= 0.5f;  // to match LearnOpenGL
                }
            });

        std::vector<glm::vec2> coords{quad.getTexCoords().begin(), quad.getTexCoords().end()};
        for (glm::vec2& coord : coords)
        {
            coord *= 2.0f;  // to test texture wrap modes
        }
        quad.setTexCoords(coords);

        return quad;
    }
}

class osc::LOGLTexturingTab::Impl final {
public:

    Impl()
    {
        m_Camera.setViewMatrixOverride(glm::mat4{1.0f});
        m_Camera.setProjectionMatrixOverride(glm::mat4{1.0f});
        Texture2D container = LoadTexture2DFromImage(
            App::resource("textures/container.jpg"),
            ColorSpace::sRGB,
            ImageFlags_FlipVertically
        );
        container.setWrapMode(osc::TextureWrapMode::Clamp);
        m_Material.setTexture("uTexture1", std::move(container));
        m_Material.setTexture(
            "uTexture2",
            LoadTexture2DFromImage(
                App::resource("textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageFlags_FlipVertically
            )
        );
    }

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
        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

private:
    UID m_TabID;
    Shader m_Shader
    {
        App::slurp("shaders/ExperimentTexturing.vert"),
        App::slurp("shaders/ExperimentTexturing.frag"),
    };
    Material m_Material{m_Shader};
    Mesh m_Mesh = GenerateMesh();
    Camera m_Camera;
};


// public API (PIMPL)

osc::CStringView osc::LOGLTexturingTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLTexturingTab::LOGLTexturingTab(std::weak_ptr<TabHost>) :
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
