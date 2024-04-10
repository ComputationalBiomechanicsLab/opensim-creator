#include "LOGLGammaTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-3.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f},
        { 3.0f, 0.0f, 0.0f},
    });

    constexpr auto c_LightColors = std::to_array<Color>({
        {0.25f, 0.25f, 0.25f, 1.0f},
        {0.50f, 0.50f, 0.50f, 1.0f},
        {0.75f, 0.75f, 0.75f, 1.0f},
        {1.00f, 1.00f, 1.00f, 1.0f},
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/Gamma";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.set_vertices({
            { 10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f, -10.0f},

            { 10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f, -10.0f},
            { 10.0f, -0.5f, -10.0f},
        });
        rv.set_tex_coords({
            {10.0f, 0.0f},
            {0.0f,  0.0f},
            {0.0f,  10.0f},

            {10.0f, 0.0f},
            {0.0f,  10.0f},
            {10.0f, 10.0f},
        });
        rv.set_normals({
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        });
        rv.set_indices({0, 2, 1, 3, 5, 4});
        return rv;
    }

    MouseCapturingCamera CreateSceneCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateFloorMaterial(IResourceLoader& rl)
    {
        Texture2D woodTexture = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/wood.png"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.frag"),
        }};
        rv.set_texture("uFloorTexture", woodTexture);
        rv.set_vec3_array("uLightPositions", c_LightPositions);
        rv.set_color_array("uLightColors", c_LightColors);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();
        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());

        // render scene
        m_Material.set_vec3("uViewPos", m_Camera.position());
        graphics::draw(m_PlaneMesh, identity<Transform>(), m_Material, m_Camera);
        m_Camera.render_to_screen();
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        ui::Text("no need to gamma correct - OSC is a gamma-corrected renderer");
        ui::End();
    }

    Material m_Material = CreateFloorMaterial(App::resource_loader());
    Mesh m_PlaneMesh = GeneratePlane();
    MouseCapturingCamera m_Camera = CreateSceneCamera();
};


// public API

CStringView osc::LOGLGammaTab::id()
{
    return c_TabStringID;
}

osc::LOGLGammaTab::LOGLGammaTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLGammaTab::LOGLGammaTab(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab& osc::LOGLGammaTab::operator=(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab::~LOGLGammaTab() noexcept = default;

UID osc::LOGLGammaTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLGammaTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLGammaTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLGammaTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLGammaTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLGammaTab::implOnDraw()
{
    m_Impl->onDraw();
}
