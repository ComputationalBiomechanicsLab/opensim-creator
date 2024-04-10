#include "LOGLFramebuffersTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Framebuffers";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.setVerts({
            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},

            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},
            { 5.0f, -0.5f, -5.0f},
        });
        rv.setTexCoords({
            {2.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 2.0f},

            {2.0f, 0.0f},
            {0.0f, 2.0f},
            {2.0f, 2.0f},
        });
        rv.setIndices({0, 2, 1,    3, 5, 4});
        return rv;
    }

    MouseCapturingCamera CreateSceneCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        return rv;
    }

    Camera CreateScreenCamera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Mat4>());
        rv.set_projection_matrix_override(identity<Mat4>());
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
        Rect viewportRect = ui::GetMainViewportWorkspaceScreenRect();
        Vec2 viewportRectDims = dimensions_of(viewportRect);
        m_RenderTexture.setDimensions(viewportRectDims);
        m_RenderTexture.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());

        // render scene
        {
            // cubes
            m_SceneRenderMaterial.set_texture("uTexture1", m_ContainerTexture);
            graphics::draw(m_CubeMesh, {.position = {-1.0f, 0.0f, -1.0f}}, m_SceneRenderMaterial, m_SceneCamera);
            graphics::draw(m_CubeMesh, {.position = { 1.0f, 0.0f, -1.0f}}, m_SceneRenderMaterial, m_SceneCamera);

            // floor
            m_SceneRenderMaterial.set_texture("uTexture1", m_MetalTexture);
            graphics::draw(m_PlaneMesh, identity<Transform>(), m_SceneRenderMaterial, m_SceneCamera);
        }
        m_SceneCamera.render_to(m_RenderTexture);

        // render via a effect sampler
        graphics::blit_to_screen(m_RenderTexture, viewportRect, m_ScreenMaterial);

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

    Texture2D m_ContainerTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    Texture2D m_MetalTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );

    Mesh m_CubeMesh = BoxGeometry{};
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f, 1, 1};

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
