#include "LOGLBasicLightingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/BasicLighting";

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

class osc::LOGLBasicLightingTab::Impl final : public StandardTabImpl {
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

        // clear screen and ensure camera has correct pixel rect
        m_Camera.set_pixel_rect(ui::get_main_viewport_workspace_screen_rect());

        // draw cube
        m_LightingMaterial.set_color("uObjectColor", m_ObjectColor);
        m_LightingMaterial.set_color("uLightColor", m_LightColor);
        m_LightingMaterial.set_vec3("uLightPos", m_LightTransform.position);
        m_LightingMaterial.set_vec3("uViewPos", m_Camera.position());
        m_LightingMaterial.set_float("uAmbientStrength", m_AmbientStrength);
        m_LightingMaterial.set_float("uDiffuseStrength", m_DiffuseStrength);
        m_LightingMaterial.set_float("uSpecularStrength", m_SpecularStrength);
        graphics::draw(m_CubeMesh, identity<Transform>(), m_LightingMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.set_color("uLightColor", m_LightColor);
        graphics::draw(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render to output (window)
        m_Camera.render_to_screen();

        // render auxiliary UI
        ui::begin_panel("controls");
        ui::draw_vec3_input("light pos", m_LightTransform.position);
        ui::draw_float_input("ambient strength", &m_AmbientStrength);
        ui::draw_float_input("diffuse strength", &m_DiffuseStrength);
        ui::draw_float_input("specular strength", &m_SpecularStrength);
        ui::draw_rgb_color_editor("object color", m_ObjectColor);
        ui::draw_rgb_color_editor("light color", m_LightColor);
        ui::end_panel();
    }

    ResourceLoader m_Loader = App::resource_loader();

    Material m_LightingMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/Lighting/BasicLighting.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/Lighting/BasicLighting.frag"),
    }};

    Material m_LightCubeMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};

    Mesh m_CubeMesh = BoxGeometry{};

    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();

    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {1.2f, 1.0f, 2.0f},
    };
    Color m_ObjectColor = {1.0f, 0.5f, 0.31f, 1.0f};
    Color m_LightColor = Color::white();
    float m_AmbientStrength = 0.01f;
    float m_DiffuseStrength = 0.6f;
    float m_SpecularStrength = 1.0f;
};


// public API

CStringView osc::LOGLBasicLightingTab::id()
{
    return c_TabStringID;
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab& osc::LOGLBasicLightingTab::operator=(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab::~LOGLBasicLightingTab() noexcept = default;

UID osc::LOGLBasicLightingTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLBasicLightingTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLBasicLightingTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLBasicLightingTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLBasicLightingTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLBasicLightingTab::impl_on_draw()
{
    m_Impl->on_draw();
}
