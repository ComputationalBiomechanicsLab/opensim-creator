#include "LOGLMultipleLightsTab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/maths/QuaternionFunctions.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/ui/mouse_capturing_camera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    // positions of cubes within the scene
    constexpr auto c_cube_positions = std::to_array<Vector3>({
        { 0.0f,  0.0f,  0.0f },
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f },
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f },
        {-1.7f,  3.0f, -7.5f },
        { 1.3f, -2.0f, -2.5f },
        { 1.5f,  2.0f, -2.5f },
        { 1.5f,  0.2f, -1.5f },
        {-1.3f,  1.0f, -1.5  },
    });

    // positions of point lights within the scene (the camera also has a spotlight)
    constexpr auto c_point_light_positions = std::to_array<Vector3>({
        { 0.7f,  0.2f,  2.0f },
        { 2.3f, -3.3f, -4.0f },
        {-4.0f,  2.0f, -12.0f},
        { 0.0f,  0.0f, -3.0f },
    });
    constexpr auto c_point_light_ambients = std::to_array<float>({0.001f, 0.001f, 0.001f, 0.001f});
    constexpr auto c_point_light_diffuses = std::to_array<float>({0.2f, 0.2f, 0.2f, 0.2f});
    constexpr auto c_point_light_speculars = std::to_array<float>({0.5f, 0.5f, 0.5f, 0.5f});
    constexpr auto c_point_light_constants = std::to_array<float>({1.0f, 1.0f, 1.0f, 1.0f});
    constexpr auto c_point_light_linears = std::to_array<float>({0.09f, 0.09f, 0.09f, 0.09f});
    constexpr auto c_point_light_quadratics = std::to_array<float>({0.032f, 0.032f, 0.032f, 0.032f});

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material create_multiple_lights_material(ResourceLoader& loader)
    {
        const Texture2D diffuse_map = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/container2.jpg"),
            ColorSpace::sRGB
        );

        const Texture2D specular_map = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/container2_specular.jpg"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/Lighting/MultipleLights.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/Lighting/MultipleLights.frag"),
        }};

        rv.set("uMaterialDiffuse", diffuse_map);
        rv.set("uMaterialSpecular", specular_map);
        rv.set("uDirLightDirection", Vector3{-0.2f, -1.0f, -0.3f});
        rv.set("uDirLightAmbient", 0.01f);
        rv.set("uDirLightDiffuse", 0.2f);
        rv.set("uDirLightSpecular", 0.4f);

        rv.set("uSpotLightAmbient", 0.0f);
        rv.set("uSpotLightDiffuse", 1.0f);
        rv.set("uSpotLightSpecular", 0.75f);

        rv.set("uSpotLightConstant", 1.0f);
        rv.set("uSpotLightLinear", 0.09f);
        rv.set("uSpotLightQuadratic", 0.032f);
        rv.set("uSpotLightCutoff", cos(45_deg));
        rv.set("uSpotLightOuterCutoff", cos(15_deg));

        rv.set_array("uPointLightPos", c_point_light_positions);
        rv.set_array("uPointLightConstant", c_point_light_constants);
        rv.set_array("uPointLightLinear", c_point_light_linears);
        rv.set_array("uPointLightQuadratic", c_point_light_quadratics);
        rv.set_array("uPointLightAmbient", c_point_light_ambients);
        rv.set_array("uPointLightDiffuse", c_point_light_diffuses);
        rv.set_array("uPointLightSpecular", c_point_light_speculars);

        return rv;
    }

    Material create_light_cube_material(ResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/LightCube.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/LightCube.frag"),
        }};
        rv.set("uLightColor", Color::white());
        return rv;
    }
}

class osc::LOGLMultipleLightsTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/Lighting/MultipleLights"; }

    explicit Impl(LOGLMultipleLightsTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        log_viewer_.open();
        perf_panel_.open();
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        camera_.on_mount();
    }

    void on_unmount()
    {
        camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return camera_.on_event(e);
    }

    void on_draw()
    {
        camera_.on_draw();

        // clear screen and ensure camera has correct pixel rect

        // setup per-frame material vals
        multiple_lights_material_.set("uViewPos", camera_.position());
        multiple_lights_material_.set("uMaterialShininess", material_shininess_);
        multiple_lights_material_.set("uSpotLightPosition", camera_.position());
        multiple_lights_material_.set("uSpotLightDirection", camera_.direction());

        // render containers
        const Vector3 axis = normalize(Vector3{1.0f, 0.3f, 0.5f});
        for (size_t i = 0; i < c_cube_positions.size(); ++i) {
            const Vector3& pos = c_cube_positions[i];
            const auto angle = i++ * 20_deg;

            graphics::draw(
                mesh_,
                {.rotation = angle_axis(angle, axis), .translation = pos},
                multiple_lights_material_,
                camera_
            );
        }

        // render lamps
        for (const Vector3& light_position : c_point_light_positions) {
            graphics::draw(mesh_, {.scale = Vector3{0.2f}, .translation = light_position}, light_cube_material_, camera_);
        }

        // render to output (window)
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        camera_.render_to_main_window();

        // render auxiliary UI
        ui::begin_panel("controls");
        ui::draw_float_input("uMaterialShininess", &material_shininess_);
        ui::end_panel();

        log_viewer_.on_draw();
        perf_panel_.on_draw();
    }

private:
    ResourceLoader loader_ = App::resource_loader();

    Material multiple_lights_material_ = create_multiple_lights_material(loader_);
    Material light_cube_material_ = create_light_cube_material(loader_);
    Mesh mesh_ = BoxGeometry{}.mesh();

    MouseCapturingCamera camera_ = create_camera();

    float material_shininess_ = 64.0f;

    LogViewerPanel log_viewer_{&owner()};
    PerfPanel perf_panel_{&owner()};
};


CStringView osc::LOGLMultipleLightsTab::id() { return Impl::static_label(); }

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLMultipleLightsTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLMultipleLightsTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLMultipleLightsTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLMultipleLightsTab::impl_on_draw() { private_data().on_draw(); }
