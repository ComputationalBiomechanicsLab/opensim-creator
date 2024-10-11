#include "LOGLBasicLightingTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLBasicLightingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/Lighting/BasicLighting"; }

    explicit Impl(LOGLBasicLightingTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {}

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
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        // draw cube
        lighting_material_.set("uObjectColor", object_color_);
        lighting_material_.set("uLightColor", light_color_);
        lighting_material_.set("uLightPos", light_transform_.position);
        lighting_material_.set("uViewPos", camera_.position());
        lighting_material_.set("uAmbientStrength", ambient_strength_);
        lighting_material_.set("uDiffuseStrength", diffuse_strength_);
        lighting_material_.set("uSpecularStrength", specular_strength_);
        graphics::draw(cube_mesh_, identity<Transform>(), lighting_material_, camera_);

        // draw lamp
        light_cube_material_.set("uLightColor", light_color_);
        graphics::draw(cube_mesh_, light_transform_, light_cube_material_, camera_);

        // render to output (window)
        camera_.render_to_screen();

        // render auxiliary UI
        ui::begin_panel("controls");
        ui::draw_vec3_input("light pos", light_transform_.position);
        ui::draw_float_input("ambient strength", &ambient_strength_);
        ui::draw_float_input("diffuse strength", &diffuse_strength_);
        ui::draw_float_input("specular strength", &specular_strength_);
        ui::draw_rgb_color_editor("object color", object_color_);
        ui::draw_rgb_color_editor("light color", light_color_);
        ui::end_panel();
    }

private:
    ResourceLoader loader_ = App::resource_loader();

    Material lighting_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/Lighting/BasicLighting.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/Lighting/BasicLighting.frag"),
    }};

    Material light_cube_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/LightCube.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/LightCube.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{};

    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();

    Transform light_transform_ = {
        .scale = Vec3{0.2f},
        .position = {1.2f, 1.0f, 2.0f},
    };
    Color object_color_ = {1.0f, 0.5f, 0.31f, 1.0f};
    Color light_color_ = Color::white();
    float ambient_strength_ = 0.01f;
    float diffuse_strength_ = 0.6f;
    float specular_strength_ = 1.0f;
};


CStringView osc::LOGLBasicLightingTab::id() { return Impl::static_label(); }

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLBasicLightingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLBasicLightingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLBasicLightingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLBasicLightingTab::impl_on_draw() { private_data().on_draw(); }
