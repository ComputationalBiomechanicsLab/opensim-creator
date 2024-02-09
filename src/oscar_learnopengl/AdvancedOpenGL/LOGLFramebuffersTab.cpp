#include "LOGLFramebuffersTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>
#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <oscar/oscar.hpp>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_PlaneVertices = std::to_array<Vec3>({
        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},

        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},
        { 5.0f, -0.5f, -5.0f},
    });

    constexpr auto c_PlaneTexCoords = std::to_array<Vec2>({
        {2.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 2.0f},

        {2.0f, 0.0f},
        {0.0f, 2.0f},
        {2.0f, 2.0f},
    });

    constexpr auto c_PlaneIndices = std::to_array<uint16_t>({
        0, 2, 1,
        3, 5, 4,
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/Framebuffers";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }

    MouseCapturingCamera CreateSceneCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Camera CreateScreenCamera()
    {
        Camera rv;
        rv.setViewMatrixOverride(Identity<Mat4>());
        rv.setProjectionMatrixOverride(Identity<Mat4>());
        return rv;
    }
}

class osc::LOGLFramebuffersTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_SceneCamera.onMount();
    }

    void implOnUnmount() final
    {
        m_SceneCamera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_SceneCamera.onEvent(e);
    }

    void implOnDraw() final
    {
        m_SceneCamera.onDraw();

        // setup render texture
        Rect viewportRect = GetMainViewportWorkspaceScreenRect();
        Vec2 viewportRectDims = Dimensions(viewportRect);
        m_RenderTexture.setDimensions(viewportRectDims);
        m_RenderTexture.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());

        // render scene
        {
            // cubes
            m_SceneRenderMaterial.setTexture("uTexture1", m_ContainerTexture);
            Graphics::DrawMesh(m_CubeMesh, {.position = {-1.0f, 0.0f, -1.0f}}, m_SceneRenderMaterial, m_SceneCamera);
            Graphics::DrawMesh(m_CubeMesh, {.position = { 1.0f, 0.0f, -1.0f}}, m_SceneRenderMaterial, m_SceneCamera);

            // floor
            m_SceneRenderMaterial.setTexture("uTexture1", m_MetalTexture);
            Graphics::DrawMesh(m_PlaneMesh, Identity<Transform>(), m_SceneRenderMaterial, m_SceneCamera);
        }
        m_SceneCamera.renderTo(m_RenderTexture);

        // render via a effect sampler
        Graphics::BlitToScreen(m_RenderTexture, viewportRect, m_ScreenMaterial);

        // auxiliary UI
        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    ResourceLoader m_Loader = App::resource_loader();

    Material m_SceneRenderMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.frag"),
    }};

    MouseCapturingCamera m_SceneCamera = CreateSceneCamera();

    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    Texture2D m_MetalTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );

    Mesh m_CubeMesh = GenerateLearnOpenGLCubeMesh();
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_QuadMesh = GenerateTexturedQuadMesh();

    RenderTexture m_RenderTexture;
    Camera m_ScreenCamera = CreateScreenCamera();
    Material m_ScreenMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.frag"),
    }};

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

CStringView osc::LOGLFramebuffersTab::id()
{
    return c_TabStringID;
}

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab& osc::LOGLFramebuffersTab::operator=(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab::~LOGLFramebuffersTab() noexcept = default;

UID osc::LOGLFramebuffersTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLFramebuffersTab::implGetName() const
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
