#include "LOGLBlendingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_WindowLocations = std::to_array<Vec3>({
        {-1.5f, 0.0f, -0.48f},
        { 1.5f, 0.0f,  0.51f},
        { 0.0f, 0.0f,  0.7f},
        {-0.3f, 0.0f, -2.3f},
        { 0.5f, 0.0f, -0.6},
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/Blending";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.set_vertices({
            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},

            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},
            { 5.0f, -0.5f, -5.0f},
        });
        rv.set_tex_coords({
            {2.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 2.0f},

            {2.0f, 0.0f},
            {0.0f, 2.0f},
            {2.0f, 2.0f},
        });
        rv.set_indices({0, 2, 1, 3, 5, 4});
        return rv;
    }

    Mesh GenerateTransparent()
    {
        Mesh rv;
        rv.set_vertices({
            {0.0f,  0.5f, 0.0f},
            {0.0f, -0.5f, 0.0f},
            {1.0f, -0.5f, 0.0f},

            {0.0f,  0.5f, 0.0f},
            {1.0f, -0.5f, 0.0f},
            {1.0f,  0.5f, 0.0f},
        });
        rv.set_tex_coords({
            {0.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 1.0f},

            {0.0f, 0.0f},
            {1.0f, 1.0f},
            {1.0f, 0.0f},
        });
        rv.set_indices({0, 1, 2, 3, 4, 5});
        return rv;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLBlendingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_BlendingMaterial.set_transparent(true);
        m_LogViewer.open();
        m_PerfPanel.open();
    }

private:
    void implOnMount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.on_mount();
    }

    void implOnUnmount() final
    {
        m_Camera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.on_event(e);
    }

    void implOnDraw() final
    {
        m_Camera.on_draw();

        // clear screen and ensure camera has correct pixel rect
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());

        // cubes
        {
            m_OpaqueMaterial.set_texture("uTexture", m_MarbleTexture);
            graphics::draw(m_CubeMesh, {.position = {-1.0f, 0.0f, -1.0f}}, m_OpaqueMaterial, m_Camera);
            graphics::draw(m_CubeMesh, {.position = { 1.0f, 0.0f, -1.0f}}, m_OpaqueMaterial, m_Camera);
        }

        // floor
        {
            m_OpaqueMaterial.set_texture("uTexture", m_MetalTexture);
            graphics::draw(m_PlaneMesh, identity<Transform>(), m_OpaqueMaterial, m_Camera);
        }

        // windows
        {
            m_BlendingMaterial.set_texture("uTexture", m_WindowTexture);
            for (Vec3 const& windowLocation : c_WindowLocations) {
                graphics::draw(m_TransparentMesh, {.position = windowLocation}, m_BlendingMaterial, m_Camera);
            }
        }

        m_Camera.render_to_screen();

        // auxiliary UI
        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_OpaqueMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Blending.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Blending.frag"),
    }};
    Material m_BlendingMaterial = m_OpaqueMaterial;
    Mesh m_CubeMesh = BoxGeometry{};
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_TransparentMesh = GenerateTransparent();
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    Texture2D m_MarbleTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/marble.jpg"),
        ColorSpace::sRGB
    );
    Texture2D m_MetalTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );
    Texture2D m_WindowTexture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/window.png"),
        ColorSpace::sRGB
    );
    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

CStringView osc::LOGLBlendingTab::id()
{
    return c_TabStringID;
}

osc::LOGLBlendingTab::LOGLBlendingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBlendingTab::LOGLBlendingTab(LOGLBlendingTab&&) noexcept = default;
osc::LOGLBlendingTab& osc::LOGLBlendingTab::operator=(LOGLBlendingTab&&) noexcept = default;
osc::LOGLBlendingTab::~LOGLBlendingTab() noexcept = default;

UID osc::LOGLBlendingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLBlendingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBlendingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBlendingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBlendingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBlendingTab::implOnDraw()
{
    m_Impl->onDraw();
}
