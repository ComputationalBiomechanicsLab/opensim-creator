#include "LOGLNormalMappingTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    // matches the quad used in LearnOpenGL's normal mapping tutorial
    Mesh generate_quad()
    {
        Mesh rv;
        rv.set_vertices({
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        rv.set_normals({
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        rv.set_tex_coords({
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        rv.set_indices({
            0, 1, 2,
            0, 2, 3,
        });
        rv.recalculate_tangents();
        return rv;
    }

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        return rv;
    }

    Material create_normal_mapping_material(IResourceLoader& loader)
    {
        const Texture2D diffuse_map = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/brickwall.jpg"),
            ColorSpace::sRGB
        );
        const Texture2D normal_map = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/brickwall_normal.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.frag"),
        }};
        rv.set("uDiffuseMap", diffuse_map);
        rv.set("uNormalMap", normal_map);

        return rv;
    }

    Material create_lightcube_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
            loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLNormalMappingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "LearnOpenGL/NormalMapping"; }

    Impl() : TabPrivate{static_label()} {}

    void on_mount()
    {
        camera_.on_mount();
    }

    void on_unmount()
    {
        camera_.on_unmount();
    }

    bool on_event(Event& e)
    {
        return camera_.on_event(e);
    }

    void on_tick()
    {
        // rotate the quad over time
        const AppClock::duration dt = App::get().frame_delta_since_startup();
        const auto angle = Degrees{-10.0 * dt.count()};
        const auto axis = UnitVec3{1.0f, 0.0f, 1.0f};
        quad_transform_.rotation = angle_axis(angle, axis);
    }

    void on_draw()
    {
        camera_.on_draw();

        // clear screen and ensure camera has correct pixel rect
        App::upd().clear_screen(Color::dark_grey());

        // draw normal-mapped quad
        {
            normal_mapping_material_.set("uLightWorldPos", light_transform_.position);
            normal_mapping_material_.set("uViewWorldPos", camera_.position());
            normal_mapping_material_.set("uEnableNormalMapping", normal_mapping_enabled_);
            graphics::draw(quad_mesh_, quad_transform_, normal_mapping_material_, camera_);
        }

        // draw light source cube
        {
            light_cube_material_.set("uLightColor", Color::white());
            graphics::draw(cube_mesh_, light_transform_, light_cube_material_, camera_);
        }

        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.render_to_screen();

        ui::begin_panel("controls");
        ui::draw_checkbox("normal mapping", &normal_mapping_enabled_);
        ui::end_panel();
    }

private:
    ResourceLoader loader_ = App::resource_loader();

    // rendering state
    Material normal_mapping_material_ = create_normal_mapping_material(loader_);
    Material light_cube_material_ = create_lightcube_material(loader_);
    Mesh cube_mesh_ = BoxGeometry{};
    Mesh quad_mesh_ = generate_quad();

    // scene state
    MouseCapturingCamera camera_ = create_camera();
    Transform quad_transform_;
    Transform light_transform_ = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool normal_mapping_enabled_ = true;
};


CStringView osc::LOGLNormalMappingTab::id() { return Impl::static_label(); }
osc::LOGLNormalMappingTab::LOGLNormalMappingTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}
void osc::LOGLNormalMappingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLNormalMappingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLNormalMappingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLNormalMappingTab::impl_on_tick() { private_data().on_tick(); }
void osc::LOGLNormalMappingTab::impl_on_draw() { private_data().on_draw(); }
