#include "RendererHelloTriangleTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Rgba32.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>

namespace
{
    osc::Mesh GenerateTriangleMesh()
    {
        glm::vec3 const points[] =
        {
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        };
        osc::Rgba32 const colors[] =
        {
            {0xff, 0x00, 0x00, 0xff},
            {0x00, 0xff, 0x00, 0xff},
            {0x00, 0x00, 0xff, 0xff},
        };
        uint16_t const indices[] = {0, 1, 2};

        osc::Mesh m;
        m.setVerts(points);
        m.setColors(colors);
        m.setIndices(indices);
        return m;
    }
}

class osc::RendererHelloTriangleTab::Impl final {
public:

    Impl()
    {
        m_Camera.setViewMatrixOverride(glm::mat4{1.0f});
        m_Camera.setProjectionMatrixOverride(glm::mat4{1.0f});
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "Hello Triangle (LearnOpenGL)";
    }

    void onDraw()
    {
        Graphics::DrawMesh(m_TriangleMesh, osc::Transform{}, m_Material, m_Camera);

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

private:
    UID m_TabID;

    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/ExperimentTriangle.vert"),
            App::slurp("shaders/ExperimentTriangle.frag"),
        },
    };
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    Camera m_Camera;
};


// public API (PIMPL)

osc::CStringView osc::RendererHelloTriangleTab::id() noexcept
{
    return "Renderer/HelloTriangle";
}

osc::RendererHelloTriangleTab::RendererHelloTriangleTab(TabHost*) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererHelloTriangleTab::RendererHelloTriangleTab(RendererHelloTriangleTab&&) noexcept = default;
osc::RendererHelloTriangleTab& osc::RendererHelloTriangleTab::operator=(RendererHelloTriangleTab&&) noexcept = default;
osc::RendererHelloTriangleTab::~RendererHelloTriangleTab() noexcept = default;

osc::UID osc::RendererHelloTriangleTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererHelloTriangleTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererHelloTriangleTab::implOnDraw()
{
    m_Impl->onDraw();
}
