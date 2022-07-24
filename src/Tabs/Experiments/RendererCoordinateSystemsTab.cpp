#include "RendererCoordinateSystemsTab.hpp"

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
#include <numeric>
#include <string>
#include <utility>

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uModelMat;
    uniform mat4 uViewMat;
    uniform mat4 uProjMat;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 FragTexCoord;

    void main()
    {
	    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
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

static glm::vec3 g_CubePositions[] =
{
    { 0.0f,  0.0f,  0.0f},
    { 2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f, -2.5f},
    {-3.8f, -2.0f, -12.3f},
    { 2.4f, -0.4f, -3.5f},
    {-1.7f,  3.0f, -7.5f},
    { 1.3f, -2.0f, -2.5f},
    { 1.5f,  2.0f, -2.5f},
    { 1.5f,  0.2f, -1.5f},
    {-1.3f,  1.0f, -1.5},
};

// generate a texture-mapped cube
static osc::experimental::Mesh GenerateMesh()
{
    auto generatedCube = osc::GenCube();
    for (glm::vec3& vert : generatedCube.verts)
    {
        vert *= 0.5f;  // to match LearnOpenGL
    }
    osc::experimental::Mesh m;
    m.setVerts(generatedCube.verts);
    m.setTexCoords(generatedCube.texcoords);
    m.setIndices(generatedCube.indices);
    return m;
}

class osc::RendererCoordinateSystemsTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setDirection({0.0f, 0.0f, -1.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Material.setTexture("uTexture1", osc::experimental::LoadTexture2DFromImageResource("container.jpg"));
        m_Material.setTexture("uTexture2", osc::experimental::LoadTexture2DFromImageResource("awesomeface.png"));
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Coordinate Systems (LearnOpenGL)";
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
        float angle = osc::App::get().getDeltaSinceAppStartup().count() * glm::radians(50.0f);
        m_Step1.rotation = glm::quat(angle * glm::vec3{0.5f, 1.0f, 0.0f});
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        osc::App::upd().clearScreen({0.2f, 0.3f, 0.3f, 1.0f});
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        ImGui::Begin("Tutorial Step");
        ImGui::Checkbox("step1", &m_ShowStep1);
        ImGui::End();

        if (m_ShowStep1)
        {
            experimental::Graphics::DrawMesh(m_Mesh, m_Step1, m_Material, m_Camera);
        }
        else
        {
            // TODO: investigate why the angles here don't match the LearnOpenGL reference image
            int i = 0;
            for (glm::vec3 const& pos : g_CubePositions)
            {
                float angle = glm::radians(i++ * 20.0f);

                Transform t;
                t.rotation = glm::quat(glm::vec3{angle * glm::vec3{1.0f, 0.3f, 0.5f}});
                t.position = pos;

                experimental::Graphics::DrawMesh(m_Mesh, t, m_Material, m_Camera);
            }
        }

        m_Camera.render();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
    experimental::Material m_Material{m_Shader};
    experimental::Mesh m_Mesh = GenerateMesh();
    experimental::Camera m_Camera;
    bool m_ShowStep1 = false;
    Transform m_Step1;
};


// public API (PIMPL)

osc::RendererCoordinateSystemsTab::RendererCoordinateSystemsTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererCoordinateSystemsTab::RendererCoordinateSystemsTab(RendererCoordinateSystemsTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererCoordinateSystemsTab& osc::RendererCoordinateSystemsTab::operator=(RendererCoordinateSystemsTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererCoordinateSystemsTab::~RendererCoordinateSystemsTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererCoordinateSystemsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererCoordinateSystemsTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererCoordinateSystemsTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererCoordinateSystemsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererCoordinateSystemsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererCoordinateSystemsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererCoordinateSystemsTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererCoordinateSystemsTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererCoordinateSystemsTab::implOnDraw()
{
    m_Impl->onDraw();
}
