#include "LOGLPBRLightingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/Lighting";

    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_LightPositions.size()> c_LightRadiances = std::to_array<Vec3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_NumRows = 7;
    constexpr int c_NumCols = 7;
    constexpr float c_CellSpacing = 2.5f;

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.frag"),
        }};
        rv.set_float("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRLightingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

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
        draw3DRender();
        draw2DUI();
    }

    void draw3DRender()
    {
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());

        m_PBRMaterial.set_vec3("uCameraWorldPos", m_Camera.position());
        m_PBRMaterial.set_vec3_array("uLightPositions", c_LightPositions);
        m_PBRMaterial.set_vec3_array("uLightColors", c_LightRadiances);

        drawSpheres();
        drawLights();

        m_Camera.render_to_screen();
    }

    void drawSpheres()
    {
        m_PBRMaterial.set_vec3("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_NumRows; ++row) {
            m_PBRMaterial.set_float("uMetallicity", static_cast<float>(row) / static_cast<float>(c_NumRows));

            for (int col = 0; col < c_NumCols; ++col) {
                float const normalizedCol = static_cast<float>(col) / static_cast<float>(c_NumCols);
                m_PBRMaterial.set_float("uRoughness", clamp(normalizedCol, 0.005f, 1.0f));

                float const x = (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing;
                float const y = (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing;
                graphics::draw(m_SphereMesh, {.position = {x, y, 0.0f}}, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        m_PBRMaterial.set_vec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (Vec3 const& pos : c_LightPositions) {
            graphics::draw(m_SphereMesh, {.scale = Vec3{0.5f}, .position = pos}, m_PBRMaterial, m_Camera);
        }
    }

    void draw2DUI()
    {
        m_PerfPanel.onDraw();
    }

    ResourceLoader m_Loader = App::resource_loader();
    MouseCapturingCamera m_Camera = CreateCamera();
    Mesh m_SphereMesh = SphereGeometry{1.0f, 64, 64};
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPBRLightingTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab& osc::LOGLPBRLightingTab::operator=(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab::~LOGLPBRLightingTab() noexcept = default;

UID osc::LOGLPBRLightingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
