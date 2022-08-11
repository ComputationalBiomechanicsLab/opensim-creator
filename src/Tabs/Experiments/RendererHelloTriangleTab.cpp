#include "RendererHelloTriangleTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <utility>

static osc::Mesh GenerateTriangleMesh()
{
    glm::vec3 points[] =
    {
        {-1.0f, -1.0f, 0.0f},  // bottom-left
        { 1.0f, -1.0f, 0.0f},  // bottom-right
        { 0.0f,  1.0f, 0.0f},  // top-middle
    };
    osc::Rgba32 colors[] =
    {
        {0xff, 0x00, 0x00, 0xff},
        {0x00, 0xff, 0x00, 0xff},
        {0x00, 0x00, 0xff, 0xff},
    };
    std::uint16_t indices[] = {0, 1, 2};

    osc::Mesh m;
    m.setVerts(points);
    m.setIndices(indices);
    m.setColors(colors);
    return m;
}

class osc::RendererHelloTriangleTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Hello Triangle (LearnOpenGL)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
    }

    void onMount()
    {
    }

    void onUnmount()
    {
    }

    bool onEvent(SDL_Event const& e)
    {
        return false;
    }

    void onTick()
    {
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        Graphics::DrawMesh(m_TriangleMesh, osc::Transform{}, m_Material, m_Camera);
        m_Camera.render();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    Shader m_Shader
    {
        App::slurp("shaders/ExperimentTriangle.vert"),
        App::slurp("shaders/ExperimentTriangle.frag"),
    };
    Material m_Material{m_Shader};
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    Camera m_Camera;
};


// public API (PIMPL)

osc::RendererHelloTriangleTab::RendererHelloTriangleTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererHelloTriangleTab::RendererHelloTriangleTab(RendererHelloTriangleTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererHelloTriangleTab& osc::RendererHelloTriangleTab::operator=(RendererHelloTriangleTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererHelloTriangleTab::~RendererHelloTriangleTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererHelloTriangleTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererHelloTriangleTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererHelloTriangleTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererHelloTriangleTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererHelloTriangleTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererHelloTriangleTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererHelloTriangleTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererHelloTriangleTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererHelloTriangleTab::implOnDraw()
{
    m_Impl->onDraw();
}
