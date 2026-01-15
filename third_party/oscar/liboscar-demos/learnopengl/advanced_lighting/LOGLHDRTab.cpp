#include "LOGLHDRTab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/PlaneGeometry.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/maths/Transform.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/ui/mouse_capturing_camera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_light_positions = std::to_array<Vector3>({
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    });

    std::array<Color, c_light_positions.size()> get_light_colors()
    {
        return std::to_array<Color>({
            to_srgb_colorspace({200.0f, 200.0f, 200.0f, 1.0f}),
            to_srgb_colorspace({0.1f, 0.0f, 0.0f, 1.0f}),
            to_srgb_colorspace({0.0f, 0.0f, 0.2f, 1.0f}),
            to_srgb_colorspace({0.0f, 0.1f, 0.0f, 1.0f}),
        });
    }

    Transform calc_corridoor_transform()
    {
        return {.scale = {2.5f, 2.5f, 27.5f}, .translation = {0.0f, 0.0f, 25.0f}};
    }

    MouseCapturingCamera create_scene_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        rv.eulers() = {0_deg, 180_deg, 0_deg};
        return rv;
    }

    Material create_scene_material(ResourceLoader& loader)
    {
        const Texture2D wood_texture = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/wood.jpg"),
            ColorSpace::sRGB
        );

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/HDR/Scene.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/HDR/Scene.frag"),
        }};
        material.set_array("uSceneLightPositions", c_light_positions);
        material.set_array("uSceneLightColors", get_light_colors());
        material.set("uDiffuseTexture", wood_texture);
        material.set("uInverseNormals", true);
        return material;
    }

    Material create_tonemap_material(ResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/HDR/Tonemap.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/HDR/Tonemap.frag"),
        }};
    }
}

class osc::LOGLHDRTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/HDR"; }

    explicit Impl(LOGLHDRTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
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
        draw_3d_scene_to_hdr_texture();
        draw_hdr_texture_via_tonemapper_to_screen();
        draw_2d_ui();
    }

private:
    void draw_3d_scene_to_hdr_texture()
    {
        // reformat intermediate HDR texture to match tab dimensions etc.
        {
            const Vector2 workspace_dimensions = ui::get_main_window_workspace_dimensions();
            const float device_pixel_ratio = App::get().main_window_device_pixel_ratio();
            const Vector2 workspace_pixel_dimensions = device_pixel_ratio * workspace_dimensions;

            RenderTextureParams params = {
                .pixel_dimensions = workspace_pixel_dimensions,
                .device_pixel_ratio = device_pixel_ratio,
                .anti_aliasing_level = App::get().anti_aliasing_level(),
            };
            if (use_16bit_format_) {
                params.color_format = ColorRenderBufferFormat::R16G16B16A16_SFLOAT;
            }

            scene_hdr_texture_.reformat(params);
        }

        graphics::draw(cube_mesh_, corridoor_transform_, scene_material_, camera_);
        camera_.render_to(scene_hdr_texture_);
    }

    void draw_hdr_texture_via_tonemapper_to_screen()
    {
        Camera orthogonal_camera;
        orthogonal_camera.set_background_color(Color::clear());
        orthogonal_camera.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        orthogonal_camera.set_projection_matrix_override(identity<Matrix4x4>());
        orthogonal_camera.set_view_matrix_override(identity<Matrix4x4>());

        tonemap_material_.set("uTexture", scene_hdr_texture_);
        tonemap_material_.set("uUseTonemap", use_tonemap_);
        tonemap_material_.set("uExposure", exposure_);

        graphics::draw(quad_mesh_, identity<Transform>(), tonemap_material_, orthogonal_camera);
        orthogonal_camera.render_to_main_window();

        tonemap_material_.unset("uTexture");
    }

    void draw_2d_ui()
    {
        ui::begin_panel("controls");
        ui::draw_checkbox("use tonemapping", &use_tonemap_);
        ui::draw_checkbox("use 16-bit colors", &use_16bit_format_);
        ui::draw_float_input("exposure", &exposure_);
        ui::draw_text("pos = %f,%f,%f", camera_.position().x, camera_.position().y, camera_.position().z);
        ui::draw_text("eulers = %f,%f,%f", camera_.eulers().x.count(), camera_.eulers().y.count(), camera_.eulers().z.count());
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();
    Material scene_material_ = create_scene_material(loader_);
    Material tonemap_material_ = create_tonemap_material(loader_);
    MouseCapturingCamera camera_ = create_scene_camera();
    Mesh cube_mesh_ = BoxGeometry{{.dimensions = Vector3{2.0f}}};
    Mesh quad_mesh_ = PlaneGeometry{{.dimensions = Vector2{2.0f}}};
    Transform corridoor_transform_ = calc_corridoor_transform();
    RenderTexture scene_hdr_texture_;
    float exposure_ = 1.0f;
    bool use_16bit_format_ = true;
    bool use_tonemap_ = true;
};


CStringView osc::LOGLHDRTab::id() { return Impl::static_label(); }

osc::LOGLHDRTab::LOGLHDRTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLHDRTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLHDRTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLHDRTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLHDRTab::impl_on_draw() { private_data().on_draw(); }
