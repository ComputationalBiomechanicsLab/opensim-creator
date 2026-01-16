#include "loglpbr_specular_irradiance_textured_tab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/render_texture.h>
#include <liboscar/graphics/texture2_d.h>
#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/geometries/plane_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/ui/mouse_capturing_camera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <array>
#include <bit>
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
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
    });

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

        RenderTexture cubemap_render_target{{
            .pixel_dimensions = {512, 512},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        // create a 90 degree cube cone projection matrix
        const Matrix4x4 projection_matrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.frag"),
        }};
        material.set("uEquirectangularMap", hdr_texture);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(projection_matrix, Vector3{}));

        Camera camera;
        graphics::draw(BoxGeometry{{.dimensions = Vector3{2.0f}}}, identity<Transform>(), material, camera);
        camera.render_to(cubemap_render_target);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemap_render_target;
    }

    RenderTexture create_irradiance_cubemap(ResourceLoader& loader, const RenderTexture& skybox)
    {
        RenderTexture irradiance_cubemap{{
            .pixel_dimensions = {32, 32},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        const Matrix4x4 captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.frag"),
        }};
        material.set("uEnvironmentMap", skybox);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(captureProjection, Vector3{}));

        Camera camera;
        graphics::draw(BoxGeometry{{.dimensions = Vector3{2.0f}}}, identity<Transform>(), material, camera);
        camera.render_to(irradiance_cubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradiance_cubemap;
    }

    Cubemap create_prefiltered_environment_map(ResourceLoader& loader, const RenderTexture& environment_map)
    {
        constexpr int level_zero_width = 128;
        static_assert(std::popcount(static_cast<unsigned>(level_zero_width)) == 1);

        RenderTexture capture_render_target{{
            .pixel_dimensions = {level_zero_width, level_zero_width},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = ColorRenderBufferFormat::R16G16B16_SFLOAT,
        }};

        const Matrix4x4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.geom"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.frag"),
        }};
        material.set("uEnvironmentMap", environment_map);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(capture_projection, Vector3{}));

        Camera camera;

        Cubemap rv{level_zero_width, TextureFormat::RGBAFloat};
        rv.set_wrap_mode(TextureWrapMode::Clamp);
        rv.set_filter_mode(TextureFilterMode::Mipmap);

        constexpr size_t max_mipmap_level = static_cast<size_t>(max(
            0,
            static_cast<int>(std::bit_width(static_cast<size_t>(level_zero_width)) - 1)  // NOLINT(readability-redundant-casting) casted to int because of LWG3656
        ));
        static_assert(max_mipmap_level == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= max_mipmap_level; ++mip) {
            const size_t mip_width = level_zero_width >> mip;
            capture_render_target.set_pixel_dimensions({static_cast<int>(mip_width), static_cast<int>(mip_width)});

            material.set("uRoughness", static_cast<float>(mip)/static_cast<float>(max_mipmap_level));

            graphics::draw(BoxGeometry{{.dimensions = Vector3{2.0f}}}, identity<Transform>(), material, camera);
            camera.render_to(capture_render_target);
            graphics::copy_texture(capture_render_target, rv, mip);
        }

        return rv;
    }

    Texture2D create_2D_brdf_lookup(ResourceLoader& loader)
    {
        // TODO: `graphics::blit` with material
        Camera camera;
        camera.set_projection_matrix_override(identity<Matrix4x4>());
        camera.set_view_matrix_override(identity<Matrix4x4>());

        graphics::draw(
            PlaneGeometry{{.dimensions = Vector2{2.0f}}},
            identity<Transform>(),
            Material{Shader{
                loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/BRDF.vert"),
                loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/BRDF.frag"),
            }},
            camera
        );

        RenderTexture render_texture{{
            .pixel_dimensions = {512, 512},
            .color_format = ColorRenderBufferFormat::R16G16_SFLOAT,
        }};
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

    Material create_material(ResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/PBR.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/PBR.frag"),
        }};
        rv.set("uAO", 1.0f);
        return rv;
    }

    struct IBLSpecularObjectTextures final {
        explicit IBLSpecularObjectTextures(ResourceLoader loader) :
            albedo_map{Image::read_into_texture(loader.open("albedo.jpg"), ColorSpace::sRGB)},
            normal_map{Image::read_into_texture(loader.open("normal.jpg"), ColorSpace::Linear, ImageLoadingFlag::TreatComponentsAsSpatialVectors)},
            metallic_map{Image::read_into_texture(loader.open("metallic.jpg"), ColorSpace::Linear)},
            roughness_map{Image::read_into_texture(loader.open("roughness.jpg"), ColorSpace::Linear)},
            ao_map{Image::read_into_texture(loader.open("ao.jpg"), ColorSpace::Linear)}
        {}

        Texture2D albedo_map;
        Texture2D normal_map;
        Texture2D metallic_map;
        Texture2D roughness_map;
        Texture2D ao_map;
    };
}

class osc::LOGLPBRSpecularIrradianceTexturedTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/PBR/SpecularIrradianceTextured"; }

    explicit Impl(LOGLPBRSpecularIrradianceTexturedTab& owner, Widget* parent) :
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
        const Rect workspace_screen_space_rect = ui::get_main_window_workspace_screen_space_rect();
        const float device_pixel_ratio = App::get().main_window_device_pixel_ratio();
        const Vector2 workspace_pixel_dimensions = device_pixel_ratio * workspace_screen_space_rect.dimensions();

        output_render_.set_pixel_dimensions(workspace_pixel_dimensions);
        output_render_.set_device_pixel_ratio(device_pixel_ratio);
        output_render_.set_anti_aliasing_level(App::get().anti_aliasing_level());

        camera_.on_draw();
        draw_3d_render();
        draw_background();
        graphics::blit_to_main_window(output_render_, workspace_screen_space_rect);
        perf_panel_.on_draw();
    }

private:
    void draw_3d_render()
    {
        set_common_material_properties();
        draw_spheres();
        draw_lights();

        camera_.render_to(output_render_);
    }

    void set_common_material_properties()
    {
        pbr_material_.set("uCameraWorldPos", camera_.position());
        pbr_material_.set_array("uLightPositions", c_light_positions);
        pbr_material_.set_array("uLightColors", c_light_radiances);
        pbr_material_.set("uIrradianceMap", irradiance_map_);
        pbr_material_.set("uPrefilterMap", prefilter_map_);
        pbr_material_.set("uMaxReflectionLOD", static_cast<float>(std::bit_width(static_cast<size_t>(prefilter_map_.width()) - 1)));
        pbr_material_.set("uBRDFLut", brdf_lookup_);
    }

    void set_material_maps(Material& material, const IBLSpecularObjectTextures& textures)
    {
        material.set("uAlbedoMap", textures.albedo_map);
        material.set("uNormalMap", textures.normal_map);
        material.set("uMetallicMap", textures.metallic_map);
        material.set("uRoughnessMap", textures.roughness_map);
        material.set("uAOMap", textures.ao_map);
    }

    void draw_spheres()
    {
        Vector3 pos = {-5.0f, 0.0f, 2.0f};
        for (const IBLSpecularObjectTextures& texture : object_textures_) {
            set_material_maps(pbr_material_, texture);
            graphics::draw(sphere_mesh_, {.translation = pos}, pbr_material_, camera_);
            pos.x += 2.0f;
        }
    }

    void draw_lights()
    {
        for (const Vector3& position : c_light_positions) {
            graphics::draw(
                sphere_mesh_,
                {.scale = Vector3{0.5f}, .translation = position},
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
        camera_.render_to(output_render_);
        camera_.set_clear_flags(CameraClearFlag::Default);
    }

    ResourceLoader loader_ = App::resource_loader();

    Texture2D texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear
    );

    std::array<IBLSpecularObjectTextures, 5> object_textures_ = std::to_array<IBLSpecularObjectTextures>({
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_demos/learnopengl/textures/pbr/rusted_iron")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_demos/learnopengl/textures/pbr/gold")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_demos/learnopengl/textures/pbr/grass")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_demos/learnopengl/textures/pbr/plastic")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_demos/learnopengl/textures/pbr/wall")},
    });

    RenderTexture projected_map_ = load_equirectangular_hdr_texture_into_cubemap(loader_);
    RenderTexture irradiance_map_ = create_irradiance_cubemap(loader_, projected_map_);
    Cubemap prefilter_map_ = create_prefiltered_environment_map(loader_, projected_map_);
    Texture2D brdf_lookup_ = create_2D_brdf_lookup(loader_);
    RenderTexture output_render_;

    Material background_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/Skybox.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/PBR/ibl_specular_textured/Skybox.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{{.dimensions = Vector3{2.0f}}};
    Material pbr_material_ = create_material(loader_);
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 64, .num_height_segments = 64}};

    MouseCapturingCamera camera_ = create_camera();

    PerfPanel perf_panel_{&owner()};
};


CStringView osc::LOGLPBRSpecularIrradianceTexturedTab::id() { return Impl::static_label(); }

osc::LOGLPBRSpecularIrradianceTexturedTab::LOGLPBRSpecularIrradianceTexturedTab(Widget* owner) :
    Tab{std::make_unique<Impl>(*this, owner)}
{}
void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_draw() { private_data().on_draw(); }
