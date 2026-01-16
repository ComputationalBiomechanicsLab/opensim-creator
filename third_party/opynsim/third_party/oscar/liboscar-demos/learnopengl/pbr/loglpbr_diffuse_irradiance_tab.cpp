#include "loglpbr_diffuse_irradiance_tab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/render_texture.h>
#include <liboscar/graphics/texture2_d.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/vector3.h>
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
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vector3, c_light_positions.size()> c_light_radiances = std::to_array<Vector3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_num_rows = 7;
    constexpr int c_num_cols = 7;
    constexpr float c_cell_spacing = 2.5f;

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture load_equirectangular_hdr_texture_into_cubemap(ResourceLoader& loader)
    {
        Texture2D hdr_texture = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear
        );
        hdr_texture.set_wrap_mode(TextureWrapMode::Clamp);
        hdr_texture.set_filter_mode(TextureFilterMode::Linear);

        RenderTexture cubemap_render_texture{{
            .pixel_dimensions = {512, 512},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        // create a 90 degree cube cone projection matrix
        const Matrix4x4 projection_matrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.frag"),
        }};
        material.set("uEquirectangularMap", hdr_texture);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(projection_matrix, Vector3{}));

        Camera camera;
        graphics::draw(
            BoxGeometry{{.dimensions = Vector3{2.0f}}},
            identity<Transform>(),
            material,
            camera
        );
        camera.render_to(cubemap_render_texture);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return cubemap_render_texture;
    }

    RenderTexture create_irradiance_cubemap(ResourceLoader& loader, const RenderTexture& skybox)
    {
        RenderTexture irradiance_cubemap{{
            .pixel_dimensions = {32, 32},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        const Matrix4x4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/Convolution.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/Convolution.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/Convolution.frag"),
        }};
        material.set("uEnvironmentMap", skybox);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(capture_projection, Vector3{}));

        Camera camera;
        graphics::draw(BoxGeometry{{.dimensions = Vector3{2.0f}}}, identity<Transform>(), material, camera);
        camera.render_to(irradiance_cubemap);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return irradiance_cubemap;
    }

    Material create_material(ResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/PBR.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/PBR.frag"),
        }};
        rv.set("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRDiffuseIrradianceTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/PBR/DiffuseIrradiance"; }

    explicit Impl(LOGLPBRDiffuseIrradianceTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        sphere_mesh_.recalculate_tangents();  // normal mapping
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
        draw_3d_render();
        draw_background();
        draw_2d_ui();
    }

private:
    void draw_3d_render()
    {
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());

        pbr_material_.set("uCameraWorldPos", camera_.position());
        pbr_material_.set_array("uLightPositions", c_light_positions);
        pbr_material_.set_array("uLightColors", c_light_radiances);
        pbr_material_.set("uIrradianceMap", irradiance_map_);

        draw_spheres();
        draw_lights();

        camera_.render_to_main_window();
    }

    void draw_spheres()
    {
        pbr_material_.set("uAlbedoColor", Vector3{0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_num_rows; ++row) {
            pbr_material_.set("uMetallicity", static_cast<float>(row) / static_cast<float>(c_num_rows));

            for (int col = 0; col < c_num_cols; ++col) {
                const float normalizedCol = static_cast<float>(col) / static_cast<float>(c_num_cols);
                pbr_material_.set("uRoughness", clamp(normalizedCol, 0.005f, 1.0f));

                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;

                graphics::draw(sphere_mesh_, {.translation = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        pbr_material_.set("uAlbedoColor", Vector3{1.0f, 1.0f, 1.0f});

        for (const Vector3& light_positions : c_light_positions) {
            graphics::draw(sphere_mesh_, {.scale = Vector3{0.5f}, .translation = light_positions}, pbr_material_, camera_);
        }
    }

    void draw_background()
    {
        background_material_.set("uEnvironmentMap", projected_map_);
        background_material_.set_depth_function(DepthFunction::LessOrEqual);  // for skybox depth trick
        graphics::draw(cube_mesh_, identity<Transform>(), background_material_, camera_);
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        camera_.set_clear_flags(CameraClearFlag::None);
        camera_.render_to_main_window();
        camera_.set_clear_flags(CameraClearFlag::Default);
    }

    void draw_2d_ui()
    {
        if (ui::begin_panel("Controls")) {
            float ao = pbr_material_.get<float>("uAO").value_or(1.0f);
            if (ui::draw_float_slider("ao", &ao, 0.0f, 1.0f)) {
                pbr_material_.set("uAO", ao);
            }
        }
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();

    Texture2D texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear
    );

    RenderTexture projected_map_ = load_equirectangular_hdr_texture_into_cubemap(loader_);
    RenderTexture irradiance_map_ = create_irradiance_cubemap(loader_, projected_map_);

    Material background_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/Background.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/diffuse_irradiance/Background.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{{.dimensions = Vector3{2.0f}}};
    Material pbr_material_ = create_material(loader_);
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 64, .num_height_segments = 64}};
    MouseCapturingCamera camera_ = create_camera();
};


CStringView osc::LOGLPBRDiffuseIrradianceTab::id() { return Impl::static_label(); }

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLPBRDiffuseIrradianceTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPBRDiffuseIrradianceTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPBRDiffuseIrradianceTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPBRDiffuseIrradianceTab::impl_on_draw() { private_data().on_draw(); }
