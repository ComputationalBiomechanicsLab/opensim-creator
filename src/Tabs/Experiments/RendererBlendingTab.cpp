#include "RendererBlendingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <utility>

static glm::vec3 const g_PlaneVertices[] =
{
    { 5.0f, -0.5f,  5.0f},
    {-5.0f, -0.5f,  5.0f},
    {-5.0f, -0.5f, -5.0f},

    { 5.0f, -0.5f,  5.0f},
    {-5.0f, -0.5f, -5.0f},
    { 5.0f, -0.5f, -5.0f},
};

static glm::vec2 const g_PlaneTexCoords[] =
{
    {2.0f, 0.0f},
    {0.0f, 0.0f},
    {0.0f, 2.0f},

    {2.0f, 0.0f},
    {0.0f, 2.0f},
    {2.0f, 2.0f},
};

static std::uint16_t const g_PlaneIndices[] = {0, 2, 1, 3, 5, 4};

static glm::vec3 const g_TransparentVerts[] =
{
    {0.0f,  0.5f, 0.0f},
    {0.0f, -0.5f, 0.0f},
    {1.0f, -0.5f, 0.0f},

    {0.0f,  0.5f, 0.0f},
    {1.0f, -0.5f, 0.0f},
    {1.0f,  0.5f, 0.0f},
};

static glm::vec2 const g_TransparentTexCoords[] =
{
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f},

    {0.0f, 0.0f},
    {1.0f, 1.0f},
    {1.0f, 0.0f},
};

static std::uint16_t const g_TransparentIndices[] = {0, 1, 2, 3, 4, 5};

static glm::vec3 const g_WindowLocations[] =
{
    {-1.5f, 0.0f, -0.48f},
    { 1.5f, 0.0f,  0.51f},
    { 0.0f, 0.0f,  0.7f},
    {-0.3f, 0.0f, -2.3f},
    { 0.5f, 0.0f, -0.6},
};

static osc::Mesh GeneratePlane()
{
    osc::Mesh rv;
    rv.setVerts(g_PlaneVertices);
    rv.setTexCoords(g_PlaneTexCoords);
    rv.setIndices(g_PlaneIndices);
    return rv;
}

static osc::Mesh GenerateTransparent()
{
    osc::Mesh rv;
    rv.setVerts(g_TransparentVerts);
    rv.setTexCoords(g_TransparentTexCoords);
    rv.setIndices(g_TransparentIndices);
    return rv;
}

class osc::RendererBlendingTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        m_BlendingMaterial.setTransparent(true);

        m_LogViewer.open();
        m_PerfPanel.open();
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Blending (LearnOpenGL)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
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
        // handle mouse capturing
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // cubes
        {
            Transform t;
            t.position = {-1.0f, 0.0f, -1.0f};

            m_OpaqueMaterial.setTexture("uTexture", m_MarbleTexture);

            osc::Graphics::DrawMesh(m_CubeMesh, t, m_OpaqueMaterial, m_Camera);

            t.position += glm::vec3{2.0f, 0.0f, 0.0f};

            osc::Graphics::DrawMesh(m_CubeMesh, t, m_OpaqueMaterial, m_Camera);
        }

        // floor
        {
            m_OpaqueMaterial.setTexture("uTexture", m_MetalTexture);
            osc::Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_OpaqueMaterial, m_Camera);
        }

        // windows
        {
            m_BlendingMaterial.setTexture("uTexture", m_WindowTexture);
            for (glm::vec3 const& loc : g_WindowLocations)
            {
                Transform t;
                t.position = loc;
                osc::Graphics::DrawMesh(m_TransparentMesh, t, m_BlendingMaterial, m_Camera);
            }
        }

        m_Camera.render();

        // auxiliary UI
        m_LogViewer.draw();
        m_PerfPanel.draw();
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    Material m_OpaqueMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentBlending.vert"),
            App::slurp("shaders/ExperimentBlending.frag"),
        }
    };
    Material m_BlendingMaterial = m_OpaqueMaterial;
    Mesh m_CubeMesh = GenLearnOpenGLCube();
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_TransparentMesh = GenerateTransparent();
    Camera m_Camera;
    Texture2D m_MarbleTexture = LoadTexture2DFromImageResource("textures/marble.jpg");
    Texture2D m_MetalTexture = LoadTexture2DFromImageResource("textures/metal.png");
    Texture2D m_WindowTexture = LoadTexture2DFromImageResource("textures/window.png");
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::RendererBlendingTab::RendererBlendingTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererBlendingTab::RendererBlendingTab(RendererBlendingTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererBlendingTab& osc::RendererBlendingTab::operator=(RendererBlendingTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererBlendingTab::~RendererBlendingTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererBlendingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererBlendingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererBlendingTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererBlendingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererBlendingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererBlendingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererBlendingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererBlendingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererBlendingTab::implOnDraw()
{
    m_Impl->onDraw();
}
