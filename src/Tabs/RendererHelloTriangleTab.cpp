#include "RendererHelloTriangleTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <cstdint>
#include <string>
#include <utility>

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;
    layout (location = 3) in vec4 aColor;

    out vec4 aVertColor;

    void main()
    {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
        aVertColor = aColor;
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    in vec4 aVertColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = aVertColor;
    }
)";

static osc::experimental::Mesh GenerateTriangleMesh()
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

    osc::experimental::Mesh m;
    m.setVerts(points);
    m.setIndices(indices);
    m.setColors(colors);
    return m;
}

class osc::RendererHelloTriangleTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setViewMatrix(glm::mat4{1.0f});  // "hello triangle" is an identity transform demo
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Hello, Triangle!";
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
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return true;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<ExperimentsScreen>();
            return true;
        }
        return false;
    }

    void onTick()
    {
        if (m_Color.r < 0.0f || m_Color.r > 1.0f)
        {
            m_FadeSpeed = -m_FadeSpeed;
        }

        m_Color.r -= osc::App::get().getDeltaSinceLastFrame().count() * m_FadeSpeed;
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        experimental::Graphics::DrawMesh(m_TriangleMesh, osc::Transform{}, m_Material, m_Camera);
        m_Camera.render();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
    experimental::Material m_Material{m_Shader};
    experimental::Mesh m_TriangleMesh = GenerateTriangleMesh();
    experimental::Camera m_Camera;
    float m_FadeSpeed = 1.0f;
    glm::vec4 m_Color = {1.0f, 0.0f, 0.0f, 1.0f};

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
