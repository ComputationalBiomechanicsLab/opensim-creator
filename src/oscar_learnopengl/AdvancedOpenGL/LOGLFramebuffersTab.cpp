#include "LOGLFramebuffersTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MaterialPropertyBlock.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <memory>

namespace
{
    constexpr auto c_PlaneVertices = osc::to_array<glm::vec3>(
    {
        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},

        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},
        { 5.0f, -0.5f, -5.0f},
    });
    constexpr auto c_PlaneTexCoords = osc::to_array<glm::vec2>(
    {
        {2.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 2.0f},

        {2.0f, 0.0f},
        {0.0f, 2.0f},
        {2.0f, 2.0f},
    });
    constexpr auto c_PlaneIndices = osc::to_array<uint16_t>(
    {
        0, 2, 1,
        3, 5, 4,
    });

    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Framebuffers";

    osc::Mesh GeneratePlane()
    {
        osc::Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    osc::Camera CreateScreenCamera()
    {
        osc::Camera rv;
        rv.setViewMatrixOverride(glm::mat4{1.0f});
        rv.setProjectionMatrixOverride(glm::mat4{1.0f});
        return rv;
    }
}

class osc::LOGLFramebuffersTab::Impl final : public osc::StandardTabBase {
public:

    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
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

    void implOnDraw() final
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
        m_RenderTexture.setDimensions(viewportRectDims);
        m_RenderTexture.setAntialiasingLevel(osc::App::get().getCurrentAntiAliasingLevel());

        // render scene
        {
            // cubes
            m_SceneRenderMaterial.setTexture("uTexture1", m_ContainerTexture);
            Transform t;
            t.position = { -1.0f, 0.0f, -1.0f };
            osc::Graphics::DrawMesh(m_CubeMesh, t, m_SceneRenderMaterial, m_SceneCamera);
            t.position = { 1.0f, 0.0f, -1.0f };
            osc::Graphics::DrawMesh(m_CubeMesh, t, m_SceneRenderMaterial, m_SceneCamera);

            // floor
            m_SceneRenderMaterial.setTexture("uTexture1", m_MetalTexture);
            osc::Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_SceneRenderMaterial, m_SceneCamera);
        }
        m_SceneCamera.renderTo(m_RenderTexture);

        // render via a effect sampler
        Graphics::BlitToScreen(m_RenderTexture, viewportRect, m_ScreenMaterial);

        // auxiliary UI
        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    Material m_SceneRenderMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.frag"),
        }
    };

    Camera m_SceneCamera = CreateSceneCamera();
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {};

    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    Texture2D m_MetalTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );

    Mesh m_CubeMesh = GenLearnOpenGLCube();
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_QuadMesh = GenTexturedQuad();

    RenderTexture m_RenderTexture;
    Camera m_ScreenCamera = CreateScreenCamera();
    Material m_ScreenMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.frag"),
        }
    };

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

osc::CStringView osc::LOGLFramebuffersTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab& osc::LOGLFramebuffersTab::operator=(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab::~LOGLFramebuffersTab() noexcept = default;

osc::UID osc::LOGLFramebuffersTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLFramebuffersTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLFramebuffersTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLFramebuffersTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLFramebuffersTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLFramebuffersTab::implOnDraw()
{
    m_Impl->onDraw();
}
