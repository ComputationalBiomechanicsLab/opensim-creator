#include "LOGLPBRSpecularIrradianceTab.h"

#include <liboscar/oscar.h>

#include <array>
#include <bit>
#include <memory>

namespace graphics = osc::graphics;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_light_positions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_light_positions.size()> c_light_radiances = std::to_array<Vec3>({
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

    RenderTexture load_equirectangular_hdr_texture_into_cubemap(
        IResourceLoader& loader)
    {
        Texture2D hdr_texture = load_texture2D_from_image(
            loader.open("oscar_demos/learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear
        );
        hdr_texture.set_wrap_mode(TextureWrapMode::Clamp);
        hdr_texture.set_filter_mode(TextureFilterMode::Linear);

        RenderTexture cubemap_render_target{{
            .dimensions = {512, 512},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        // create a 90 degree cube cone projection matrix
        const Mat4 projection_matrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.frag"),
        }};
        material.set("uEquirectangularMap", hdr_texture);
        material.set_array(
            "uShadowMatrices",
            calc_cubemap_view_proj_matrices(projection_matrix, Vec3{})
        );

        Camera camera;
        graphics::draw(BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}}, identity<Transform>(), material, camera);
        camera.render_to(cubemap_render_target);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemap_render_target;
    }

    RenderTexture create_irradiance_cubemap(
        IResourceLoader& loader,
        const RenderTexture& skybox)
    {
        RenderTexture irradiance_cubemap{{
            .dimensions = {32, 32},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        const Mat4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.frag"),
        }};
        material.set("uEnvironmentMap", skybox);
        material.set_array(
            "uShadowMatrices",
            calc_cubemap_view_proj_matrices(capture_projection, Vec3{})
        );

        Camera camera;
        graphics::draw(BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}}, identity<Transform>(), material, camera);
        camera.render_to(irradiance_cubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradiance_cubemap;
    }

    Cubemap create_prefiltered_environment_map(
        IResourceLoader& loader,
        const RenderTexture& environment_map)
    {
        const int level_zero_width = 128;
        static_assert(std::popcount(static_cast<unsigned>(level_zero_width)) == 1);

        RenderTexture capture_render_texture{{
            .dimensions = {level_zero_width, level_zero_width},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        const Mat4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/Prefilter.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/Prefilter.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/Prefilter.frag"),
        }};
        material.set("uEnvironmentMap", environment_map);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(capture_projection, Vec3{}));

        Camera camera;

        Cubemap rv{level_zero_width, TextureFormat::RGBFloat};  // TODO: add support for TextureFormat:::RGFloat16
        rv.set_wrap_mode(TextureWrapMode::Clamp);
        rv.set_filter_mode(TextureFilterMode::Mipmap);

        const auto max_mipmap_level = static_cast<size_t>(max(
            0,
            static_cast<int>(std::bit_width(static_cast<size_t>(level_zero_width))) - 1
        ));
        static_assert(max_mipmap_level == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= max_mipmap_level; ++mip) {
            const size_t mip_width = level_zero_width >> mip;
            capture_render_texture.set_dimensions({static_cast<int>(mip_width), static_cast<int>(mip_width)});

            const float mip_roughness = static_cast<float>(mip)/static_cast<float>(max_mipmap_level);
            material.set("uRoughness", mip_roughness);

            graphics::draw(BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}}, identity<Transform>(), material, camera);
            camera.render_to(capture_render_texture);
            graphics::copy_texture(capture_render_texture, rv, mip);
        }

        return rv;
    }

    Texture2D create_2D_brdf_lookup(
        IResourceLoader& loader)
    {
        RenderTexture render_texture{{
            .dimensions = {512, 512},
            .color_format = ColorRenderBufferFormat::R16G16_SFLOAT,
        }};

        const Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/BRDF.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/BRDF.frag"),
        }};

        // TODO: graphics::blit with material
        Camera camera;
        camera.set_projection_matrix_override(identity<Mat4>());
        camera.set_view_matrix_override(identity<Mat4>());

        graphics::draw(PlaneGeometry{{.width = 2.0f, .height = 2.0f}}, identity<Transform>(), material, camera);
        camera.render_to(render_texture);

        Texture2D rv{
            {512, 512},
            TextureFormat::RGFloat,  // TODO: add support for TextureFormat:::RGFloat16
            ColorSpace::Linear,
            TextureWrapMode::Clamp,
            TextureFilterMode::Linear,
        };
        graphics::copy_texture(render_texture, rv);
        return rv;
    }

    Material create_material(
        IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/PBR.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/PBR.frag"),
        }};
        rv.set<float>("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRSpecularIrradianceTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/PBR/SpecularIrradiance"; }

    explicit Impl(LOGLPBRSpecularIrradianceTab& owner, Widget* parent) :
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
        const Rect workspace_screenspace_rect = ui::get_main_window_workspace_screenspace_rect();
        const float device_pixel_ratio = App::get().main_window_device_pixel_ratio();
        const Vec2 workspace_pixel_dimensions = device_pixel_ratio * dimensions_of(workspace_screenspace_rect);

        output_render_texture_.set_dimensions(workspace_pixel_dimensions);
        output_render_texture_.set_device_pixel_ratio(device_pixel_ratio);
        output_render_texture_.set_anti_aliasing_level(App::get().anti_aliasing_level());

        camera_.on_draw();
        draw_3d_render();
        draw_background();
        graphics::blit_to_screen(output_render_texture_, workspace_screenspace_rect);
        draw_2d_ui();
        perf_panel_.on_draw();
    }

private:
    void draw_3d_render()
    {
        pbr_material_.set("uCameraWorldPos", camera_.position());
        pbr_material_.set_array("uLightPositions", c_light_positions);
        pbr_material_.set_array("uLightColors", c_light_radiances);
        pbr_material_.set("uIrradianceMap", irradiance_map_);
        pbr_material_.set("uPrefilterMap", prefilter_map_);
        pbr_material_.set("uMaxReflectionLOD", static_cast<float>(std::bit_width(static_cast<size_t>(prefilter_map_.width()) - 1)));
        pbr_material_.set("uBRDFLut", brdf_lookup_);

        draw_spheres();
        draw_lights();

        camera_.render_to(output_render_texture_);
    }

    void draw_spheres()
    {
        pbr_material_.set("uAlbedoColor", Vec3{0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_num_rows; ++row) {
            pbr_material_.set("uMetallicity", static_cast<float>(row) / static_cast<float>(c_num_rows));

            for (int col = 0; col < c_num_cols; ++col) {
                const float normalizedCol = static_cast<float>(col) / static_cast<float>(c_num_cols);
                pbr_material_.set("uRoughness", clamp(normalizedCol, 0.005f, 1.0f));

                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;

                graphics::draw(sphere_mesh_, {.position = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        pbr_material_.set("uAlbedoColor", Vec3{1.0f, 1.0f, 1.0f});

        for (const Vec3& pos : c_light_positions) {
            graphics::draw(
                sphere_mesh_,
                {.scale = Vec3{0.5f}, .position = pos},
                pbr_material_,
                camera_
            );
        }
    }

    void draw_background()
    {
        background_material_.set("uEnvironmentMap", projected_map_);
        background_material_.set_depth_function(DepthFunction::LessOrEqual);  // for skybox depth trick
        graphics::draw(cube_mesh_, identity<Transform>(), background_material_, camera_);
        camera_.set_clear_flags(CameraClearFlag::None);
        camera_.render_to(output_render_texture_);
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

    Texture2D texture_ = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear
    );

    RenderTexture projected_map_ = load_equirectangular_hdr_texture_into_cubemap(loader_);
    RenderTexture irradiance_map_ = create_irradiance_cubemap(loader_, projected_map_);
    Cubemap prefilter_map_ = create_prefiltered_environment_map(loader_, projected_map_);
    Texture2D brdf_lookup_ = create_2D_brdf_lookup(loader_);
    RenderTexture output_render_texture_;

    Material background_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/Skybox.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular/Skybox.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}};
    Material pbr_material_ = create_material(loader_);
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 64, .num_height_segments = 64}};

    MouseCapturingCamera camera_ = create_camera();

    PerfPanel perf_panel_{&owner()};
};


CStringView osc::LOGLPBRSpecularIrradianceTab::id() { return Impl::static_label(); }

osc::LOGLPBRSpecularIrradianceTab::LOGLPBRSpecularIrradianceTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLPBRSpecularIrradianceTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPBRSpecularIrradianceTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPBRSpecularIrradianceTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPBRSpecularIrradianceTab::impl_on_draw() { private_data().on_draw(); }
