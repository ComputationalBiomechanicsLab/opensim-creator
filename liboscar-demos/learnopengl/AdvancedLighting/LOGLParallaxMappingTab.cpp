#include "LOGLParallaxMappingTab.h"

#include <liboscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        return rv;
    }

    Material create_parallax_mapping_material(IResourceLoader& loader)
    {
        const Texture2D diffuse_map = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/bricks2.jpg"),
            ColorSpace::sRGB
        );
        const Texture2D normal_map = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/bricks2_normal.jpg"),
            ColorSpace::Linear,
            ImageLoadingFlag::TreatComponentsAsSpatialVectors
        );
        const Texture2D displacement_map = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/bricks2_disp.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ParallaxMapping.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ParallaxMapping.frag"),
        }};
        rv.set("uDiffuseMap", diffuse_map);
        rv.set("uNormalMap", normal_map);
        rv.set("uDisplacementMap", displacement_map);
        rv.set("uHeightScale", 0.1f);
        return rv;
    }

    Material create_lightcube_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/LightCube.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLParallaxMappingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/ParallaxMapping"; }

    explicit Impl(LOGLParallaxMappingTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        quad_mesh_.recalculate_tangents();  // needed for parallax mapping
    }

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

    void on_draw()
    {
        camera_.on_draw();

        // clear screen and ensure camera has correct pixel rect
        App::upd().clear_main_window(Color::dark_grey());

        // draw normal-mapped quad
        {
            parallax_mapping_material_.set("uLightWorldPos", light_transform_.position);
            parallax_mapping_material_.set("uViewWorldPos", camera_.position());
            parallax_mapping_material_.set("uEnableMapping", parallax_mapping_enabled_);
            graphics::draw(quad_mesh_, quad_transform_, parallax_mapping_material_, camera_);
        }

        // draw light source cube
        {
            light_cube_material_.set("uLightColor", Color::white());
            graphics::draw(cube_mesh_, light_transform_, light_cube_material_, camera_);
        }

        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        camera_.render_to_main_window();

        ui::begin_panel("controls");
        ui::draw_checkbox("normal mapping", &parallax_mapping_enabled_);
        ui::end_panel();
    }

private:
    ResourceLoader loader_ = App::resource_loader();

    // rendering state
    Material parallax_mapping_material_ = create_parallax_mapping_material(loader_);
    Material light_cube_material_ = create_lightcube_material(loader_);
    Mesh cube_mesh_ = BoxGeometry{};
    Mesh quad_mesh_ = PlaneGeometry{{.dimensions = Vec2{2.0f}}};

    // scene state
    MouseCapturingCamera camera_ = create_camera();
    Transform quad_transform_;
    Transform light_transform_ = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool parallax_mapping_enabled_ = true;
};


CStringView osc::LOGLParallaxMappingTab::id() { return Impl::static_label(); }

osc::LOGLParallaxMappingTab::LOGLParallaxMappingTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLParallaxMappingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLParallaxMappingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLParallaxMappingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLParallaxMappingTab::impl_on_draw() { private_data().on_draw(); }
