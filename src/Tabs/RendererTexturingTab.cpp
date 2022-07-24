#include "RendererTexturingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
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

static char g_VertexShader[] =
R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 FragTexCoord;

    void main()
    {
	    gl_Position = vec4(aPos, 1.0);
	    FragTexCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    uniform sampler2D uTexture1;
    uniform sampler2D uTexture2;

    in vec2 FragTexCoord;
    out vec4 FragColor;

    void main()
    {
	    FragColor = mix(texture(uTexture1, FragTexCoord), texture(uTexture2, FragTexCoord), 0.2);
    }
)";

static osc::experimental::Mesh GenerateMesh()
{
    glm::vec3 points[] =
    {
        { 0.5f,  0.5f, 0.0f}, // top right
        { 0.5f, -0.5f, 0.0f}, // bottom right
        {-0.5f, -0.5f, 0.0f}, // bottom left
        {-0.5f,  0.5f, 0.0f}, // top left
    };
    glm::vec2 coords[] =
    {
        {1.0f, 1.0f}, // top right
        {1.0f, 0.0f}, // bottom right
        {0.0f, 0.0f}, // bottom left
        {0.0f, 1.0f}, // top left
    };
    std::uint16_t indices[] = {0, 2, 1, 0, 3, 2};

    for (glm::vec2& coord : coords)
    {
        coord *= 2.0f;  // to test texture wrap modes
    }

    osc::experimental::Mesh m;
    m.setVerts(points);
    m.setTexCoords(coords);
    m.setIndices(indices);
    return m;
}

static osc::experimental::Texture2D LoadTexture(std::string_view resource)
{
    osc::Rgba32Image img = osc::LoadImageRgba32(osc::App::get().resource(resource));
    osc::experimental::Texture2D rv{img.Dimensions.x, img.Dimensions.y, img.Pixels};
    return rv;
}

class osc::RendererTexturingTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        auto container = LoadTexture("container.jpg");
        container.setWrapMode(osc::experimental::TextureWrapMode::Clamp);
        m_Material.setTexture("uTexture1", container);
        m_Material.setTexture("uTexture2", LoadTexture("awesomeface.png"));
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Textured Rectangle";
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
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
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
