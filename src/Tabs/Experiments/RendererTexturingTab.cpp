#include "RendererTexturingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <string>
#include <utility>

static osc::experimental::Mesh GenerateMesh()
{
    auto quad = osc::GenTexturedQuad();

    for (glm::vec3& vert : quad.verts)
    {
        vert *= 0.5f;  // to match LearnOpenGL
    }
    for (glm::vec2& coord : quad.texcoords)
    {
        coord *= 2.0f;  // to test texture wrap modes
    }

    osc::experimental::Mesh m;
    m.setVerts(quad.verts);
    m.setTexCoords(quad.texcoords);
    m.setIndices(quad.indices);
    return m;
}

class osc::RendererTexturingTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        auto container = osc::experimental::LoadTexture2DFromImageResource("container.jpg");
        container.setWrapMode(osc::experimental::TextureWrapMode::Clamp);
        m_Material.setTexture("uTexture1", std::move(container));
        m_Material.setTexture("uTexture2", osc::experimental::LoadTexture2DFromImageResource("awesomeface.png"));
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Textures (LearnOpenGL)";
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
        experimental::Graphics::DrawMesh(m_Mesh, osc::Transform{}, m_Material, m_Camera);
        m_Camera.render();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    experimental::Shader m_Shader
    {
        osc::App::get().slurpResource("shaders/ExperimentTexturing.vert").c_str(),
        osc::App::get().slurpResource("shaders/ExperimentTexturing.frag").c_str(),
    };
    experimental::Material m_Material{m_Shader};
    experimental::Mesh m_Mesh = GenerateMesh();
    experimental::Camera m_Camera;
};


// public API (PIMPL)

osc::RendererTexturingTab::RendererTexturingTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererTexturingTab::RendererTexturingTab(RendererTexturingTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererTexturingTab& osc::RendererTexturingTab::operator=(RendererTexturingTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererTexturingTab::~RendererTexturingTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererTexturingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererTexturingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererTexturingTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererTexturingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererTexturingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererTexturingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererTexturingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererTexturingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererTexturingTab::implOnDraw()
{
    m_Impl->onDraw();
}
