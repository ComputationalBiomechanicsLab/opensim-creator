#include "LOGLLightingMapsTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/LightingMaps";

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        return rv;
    }

    Material CreateLightMappingMaterial(IResourceLoader& rl)
    {
        Texture2D diffuseMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/container2.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Texture2D specularMap = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/container2_specular.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.vert"),
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.frag"),
        }};
        rv.set_texture("uMaterialDiffuse", diffuseMap);
        rv.set_texture("uMaterialSpecular", specularMap);
        return rv;
    }
}

class osc::LOGLLightingMapsTab::Impl final : public StandardTabImpl {
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
        App::upd().clear_screen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw cube
        m_LightingMapsMaterial.set_vec3("uViewPos", m_Camera.position());
        m_LightingMapsMaterial.set_vec3("uLightPos", m_LightTransform.position);
        m_LightingMapsMaterial.set_float("uLightAmbient", m_LightAmbient);
        m_LightingMapsMaterial.set_float("uLightDiffuse", m_LightDiffuse);
        m_LightingMapsMaterial.set_float("uLightSpecular", m_LightSpecular);
        m_LightingMapsMaterial.set_float("uMaterialShininess", m_MaterialShininess);
        graphics::draw(m_Mesh, identity<Transform>(), m_LightingMapsMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.set_color("uLightColor", Color::white());
        graphics::draw(m_Mesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render 3D scene
        m_Camera.set_pixel_rect(ui::get_main_viewport_workspace_screen_rect());
        m_Camera.render_to_screen();

        // render 2D UI
        ui::begin_panel("controls");
        ui::draw_vec3_input("uLightPos", m_LightTransform.position);
        ui::draw_float_input("uLightAmbient", &m_LightAmbient);
        ui::draw_float_input("uLightDiffuse", &m_LightDiffuse);
        ui::draw_float_input("uLightSpecular", &m_LightSpecular);
        ui::draw_float_input("uMaterialShininess", &m_MaterialShininess);
        ui::end_panel();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_LightingMapsMaterial = CreateLightMappingMaterial(m_Loader);
    Material m_LightCubeMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};
    Mesh m_Mesh = BoxGeometry{};
    MouseCapturingCamera m_Camera = CreateCamera();

    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.4f, 0.4f, 2.0f},
    };
    float m_LightAmbient = 0.02f;
    float m_LightDiffuse = 0.4f;
    float m_LightSpecular = 1.0f;
    float m_MaterialShininess = 64.0f;
};


// public API

CStringView osc::LOGLLightingMapsTab::id()
{
    return c_TabStringID;
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab& osc::LOGLLightingMapsTab::operator=(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab::~LOGLLightingMapsTab() noexcept = default;

UID osc::LOGLLightingMapsTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLLightingMapsTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLLightingMapsTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLLightingMapsTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLLightingMapsTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLLightingMapsTab::impl_on_draw()
{
    m_Impl->on_draw();
}
