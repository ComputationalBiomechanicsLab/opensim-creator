#include "RendererTexturingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
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
#include <glm/vec4.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <string>
#include <utility>

static osc::Mesh GenerateMesh()
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

class osc::RendererTexturingTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        Texture2D container = osc::LoadTexture2DFromImageResource("textures/container.jpg", ImageFlags_FlipVertically);
        container.setWrapMode(osc::TextureWrapMode::Clamp);
        m_Material.setTexture("uTexture1", std::move(container));
        m_Material.setTexture("uTexture2", osc::LoadTexture2DFromImageResource("textures/awesomeface.png", ImageFlags_FlipVertically));
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
        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);
        m_Camera.render();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
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
