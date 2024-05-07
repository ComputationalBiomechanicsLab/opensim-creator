#include "LOGLPBRLightingTexturedTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/LightingTextured";

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
        Texture2D albedo = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/albedo.png"),
            ColorSpace::sRGB
        );
        Texture2D normal = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/normal.png"),
            ColorSpace::Linear
        );
        Texture2D metallic = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/metallic.png"),
            ColorSpace::Linear
        );
        Texture2D roughness = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/roughness.png"),
            ColorSpace::Linear
        );
        Texture2D ao = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/ao.png"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting_textured/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting_textured/PBR.frag"),
        }};
        rv.set_texture("uAlbedoMap", albedo);
        rv.set_texture("uNormalMap", normal);
        rv.set_texture("uMetallicMap", metallic);
        rv.set_texture("uRoughnessMap", roughness);
        rv.set_texture("uAOMap", ao);
        rv.set_vec3_array("uLightWorldPositions", c_LightPositions);
        rv.set_vec3_array("uLightRadiances", c_LightRadiances);
        return rv;
    }
}

class osc::LOGLPBRLightingTexturedTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.on_mount();
    }

    void impl_on_unmount() final
    {
        m_Camera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(SDL_Event const& e) final
    {
        return m_Camera.on_event(e);
    }

    void impl_on_draw() final
    {
        m_Camera.on_draw();
        draw3DRender();
        m_PerfPanel.on_draw();
    }

    void draw3DRender()
    {
        m_Camera.set_pixel_rect(ui::get_main_viewport_workspace_screen_rect());

        m_PBRMaterial.set_vec3("uCameraWorldPosition", m_Camera.position());

        drawSpheres();
        drawLights();

        m_Camera.render_to_screen();
    }

    void drawSpheres()
    {
        for (int row = 0; row < c_NumRows; ++row) {
            for (int col = 0; col < c_NumCols; ++col) {
                float const x = (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing;
                float const y = (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing;
                graphics::draw(m_SphereMesh, {.position = {x, y, 0.0f}}, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        for (Vec3 const& pos : c_LightPositions) {
            graphics::draw(m_SphereMesh, {.scale = Vec3{0.5f}, .position = pos}, m_PBRMaterial, m_Camera);
        }
    }

    ResourceLoader m_Loader = App::resource_loader();
    MouseCapturingCamera m_Camera = CreateCamera();
    Mesh m_SphereMesh = SphereGeometry{1.0f, 64, 64};
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPBRLightingTexturedTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab& osc::LOGLPBRLightingTexturedTab::operator=(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab::~LOGLPBRLightingTexturedTab() noexcept = default;

UID osc::LOGLPBRLightingTexturedTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLPBRLightingTexturedTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLPBRLightingTexturedTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLPBRLightingTexturedTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLPBRLightingTexturedTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLPBRLightingTexturedTab::impl_on_draw()
{
    m_Impl->on_draw();
}
