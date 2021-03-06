#include "RendererFramebuffersTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Geometry.hpp"
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

// generate a texture-mapped cube
static osc::experimental::Mesh GenerateCubeMesh()
{
    osc::MeshData cube = osc::GenCube();

    for (glm::vec3& vert : cube.verts)
    {
        vert *= 0.5f;  // makes the verts match LearnOpenGL
    }

    return osc::experimental::LoadMeshFromMeshData(cube);
}

static osc::experimental::Mesh GenerateQuadMesh()
{
    return osc::experimental::LoadMeshFromMeshData(osc::GenTexturedQuad());
}

static osc::experimental::Mesh GeneratePlane()
{
    osc::experimental::Mesh rv;
    rv.setVerts(g_PlaneVertices);
    rv.setTexCoords(g_PlaneTexCoords);
    rv.setIndices(g_PlaneIndices);
    return rv;
}

class osc::RendererFramebuffersTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{ parent }
    {
        m_SceneCamera.setPosition({0.0f, 0.0f, 3.0f});
        m_SceneCamera.setCameraFOV(glm::radians(45.0f));
        m_SceneCamera.setNearClippingPlane(0.1f);
        m_SceneCamera.setFarClippingPlane(100.0f);

        m_ScreenCamera.setViewMatrix(glm::mat4{1.0f});
        m_ScreenCamera.setProjectionMatrix(glm::mat4{1.0f});
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Frame Buffers (LearnOpenGL)";
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
            UpdateEulerCameraFromImGuiUserInput(m_SceneCamera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // setup render texture
        osc::Rect viewportRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 viewportRectDims = osc::Dimensions(viewportRect);
        osc::experimental::RenderTextureDescriptor desc = osc::experimental::RenderTextureDescriptor
        {
            static_cast<int>(viewportRectDims.x),
            static_cast<int>(viewportRectDims.y),
        };
        desc.setAntialiasingLevel(osc::App::get().getMSXAASamplesRecommended());

        m_SceneCamera.setTexture(desc);

        // render scene
        {
            // cubes
            m_SceneRenderMaterial.setTexture("uTexture1", m_ContainerTexture);
            Transform t;
            t.position = { -1.0f, 0.0f, -1.0f };
            osc::experimental::Graphics::DrawMesh(m_CubeMesh, t, m_SceneRenderMaterial, m_SceneCamera);
            t.position = { 1.0f, 0.0f, -1.0f };
            osc::experimental::Graphics::DrawMesh(m_CubeMesh, t, m_SceneRenderMaterial, m_SceneCamera);

            // floor
            m_SceneRenderMaterial.setTexture("uTexture1", m_MetalTexture);
            osc::experimental::Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_SceneRenderMaterial, m_SceneCamera);
        }
        m_SceneCamera.render();

        // render screen quad
        {
            m_ScreenMaterial.setRenderTexture("uScreenTexture", *m_SceneCamera.getTexture());
            osc::experimental::Graphics::DrawMesh(m_QuadMesh, Transform{}, m_ScreenMaterial, m_ScreenCamera);
        }

        m_ScreenCamera.setPixelRect(viewportRect);
        m_ScreenCamera.render();  // renders to screen
        m_ScreenMaterial.clearRenderTexture("uScreenTexture");  // TODO: perf

        // auxiliary UI
        m_LogViewer.draw();
        m_PerfPanel.draw();
    }

private:
    UID m_ID;
    TabHost* m_Parent;

    experimental::Material m_SceneRenderMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentFrameBuffers.vert"),
            App::slurp("shaders/ExperimentFrameBuffers.frag"),
        }
    };

    experimental::Camera m_SceneCamera;
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = { 0.0f, 0.0f, 0.0f };

    experimental::Texture2D m_ContainerTexture = osc::experimental::LoadTexture2DFromImageResource("container.jpg");
    experimental::Texture2D m_MetalTexture = osc::experimental::LoadTexture2DFromImageResource("textures/metal.png");

    experimental::Mesh m_CubeMesh = GenerateCubeMesh();
    experimental::Mesh m_PlaneMesh = GeneratePlane();
    experimental::Mesh m_QuadMesh = GenerateQuadMesh();

    experimental::Camera m_ScreenCamera;
    experimental::Material m_ScreenMaterial
    {
        experimental::Shader
        {
            App::slurp("shaders/ExperimentFrameBuffersScreen.vert"),
            App::slurp("shaders/ExperimentFrameBuffersScreen.frag"),
        }
    };

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::RendererFramebuffersTab::RendererFramebuffersTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererFramebuffersTab::RendererFramebuffersTab(RendererFramebuffersTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererFramebuffersTab& osc::RendererFramebuffersTab::operator=(RendererFramebuffersTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererFramebuffersTab::~RendererFramebuffersTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererFramebuffersTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererFramebuffersTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererFramebuffersTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererFramebuffersTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererFramebuffersTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererFramebuffersTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererFramebuffersTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererFramebuffersTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererFramebuffersTab::implOnDraw()
{
    m_Impl->onDraw();
}
