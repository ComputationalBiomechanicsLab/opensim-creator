#include "LOGLPBRSpecularIrradianceTexturedTab.h"

#include <SDL_events.h>
#include <oscar/oscar.h>

#include <array>
#include <bit>
#include <utility>

namespace graphics = osc::graphics;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/PBR/SpecularIrradianceTextured";

    constexpr auto c_light_positions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_light_positions.size()> c_light_radiances = std::to_array<Vec3>({
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
    });

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture load_equirectangular_hdr_texture_into_cubemap(ResourceLoader& loader)
    {
        Texture2D hdr_texture = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear,
            ImageLoadingFlags::FlipVertically
        );
        hdr_texture.set_wrap_mode(TextureWrapMode::Clamp);
        hdr_texture.set_filter_mode(TextureFilterMode::Linear);

        RenderTexture cubemap_render_target{{
            .dimensions = {512, 512},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = RenderTextureFormat::RGBFloat16,
        }};

        // create a 90 degree cube cone projection matrix
        const Mat4 projection_matrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.geom"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.frag"),
        }};
        material.set("uEquirectangularMap", hdr_texture);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(projection_matrix, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(cubemap_render_target);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemap_render_target;
    }

    RenderTexture create_irradiance_cubemap(ResourceLoader& loader, const RenderTexture& skybox)
    {
        RenderTexture irradiance_cubemap{{
            .dimensions = {32, 32},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = RenderTextureFormat::RGBFloat16,
        }};

        const Mat4 captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.geom"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.frag"),
        }};
        material.set("uEnvironmentMap", skybox);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(captureProjection, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(irradiance_cubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradiance_cubemap;
    }

    Cubemap create_prefiltered_environment_map(ResourceLoader& loader, const RenderTexture& environment_map)
    {
        constexpr int level_zero_width = 128;
        static_assert(std::popcount(static_cast<unsigned>(level_zero_width)) == 1);

        RenderTexture capture_render_target{{
            .dimensions = {level_zero_width, level_zero_width},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = RenderTextureFormat::RGBFloat16
        }};

        const Mat4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.geom"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.frag"),
        }};
        material.set("uEnvironmentMap", environment_map);
        material.set_array("uShadowMatrices", calc_cubemap_view_proj_matrices(capture_projection, Vec3{}));

        Camera camera;

        Cubemap rv{level_zero_width, TextureFormat::RGBAFloat};
        rv.set_wrap_mode(TextureWrapMode::Clamp);
        rv.set_filter_mode(TextureFilterMode::Mipmap);

        constexpr size_t max_mipmap_level = static_cast<size_t>(max(
            0,
            static_cast<int>(std::bit_width(static_cast<size_t>(level_zero_width))) - 1
        ));
        static_assert(max_mipmap_level == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= max_mipmap_level; ++mip) {
            const size_t mip_width = level_zero_width >> mip;
            capture_render_target.set_dimensions({static_cast<int>(mip_width), static_cast<int>(mip_width)});

           material.set("uRoughness", static_cast<float>(mip)/static_cast<float>(max_mipmap_level));

            graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
            camera.render_to(capture_render_target);
            graphics::copy_texture(capture_render_target, rv, mip);
        }

        return rv;
    }

    Texture2D create_2D_brdf_lookup(ResourceLoader& loader)
    {
        // TODO: `graphics::blit` with material
        Camera camera;
        camera.set_projection_matrix_override(identity<Mat4>());
        camera.set_view_matrix_override(identity<Mat4>());

        graphics::draw(
            PlaneGeometry{2.0f, 2.0f, 1, 1},
            identity<Transform>(),
            Material{Shader{
                loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.vert"),
                loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.frag"),
            }},
            camera
        );

        RenderTexture render_texture{{.dimensions = {512, 512}, .color_format = RenderTextureFormat::RGFloat16}};
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
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.frag"),
        }};
        rv.set("uAO", 1.0f);
        return rv;
    }

    struct IBLSpecularObjectTextures final {
        explicit IBLSpecularObjectTextures(ResourceLoader loader) :
            albedo_map{load_texture2D_from_image(loader.open("albedo.png"), ColorSpace::sRGB)},
            normal_map{load_texture2D_from_image(loader.open("normal.png"), ColorSpace::Linear)},
            metallic_map{load_texture2D_from_image(loader.open("metallic.png"), ColorSpace::Linear)},
            roughness_map{load_texture2D_from_image(loader.open("roughness.png"), ColorSpace::Linear)},
            ao_map{load_texture2D_from_image(loader.open("ao.png"), ColorSpace::Linear)}
        {}

        Texture2D albedo_map;
        Texture2D normal_map;
        Texture2D metallic_map;
        Texture2D roughness_map;
        Texture2D ao_map;
    };
}

class osc::LOGLPBRSpecularIrradianceTexturedTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        camera_.on_mount();
    }

    void impl_on_unmount() final
    {
        camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(const SDL_Event& e) final
    {
        return camera_.on_event(e);
    }

    void impl_on_draw() final
    {
        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();
        output_render_.set_dimensions(dimensions_of(viewport_screenspace_rect));
        output_render_.set_anti_aliasing_level(App::get().anti_aliasing_level());

        camera_.on_draw();
        draw_3D_render();
        draw_background();
        graphics::blit_to_screen(output_render_, viewport_screenspace_rect);
        perf_panel_.on_draw();
    }

    void draw_3D_render()
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
        Vec3 pos = {-5.0f, 0.0f, 2.0f};
        for (const IBLSpecularObjectTextures& texture : object_textures_) {
            set_material_maps(pbr_material_, texture);
            graphics::draw(sphere_mesh_, {.position = pos}, pbr_material_, camera_);
            pos.x += 2.0f;
        }
    }

    void draw_lights()
    {
        for (const Vec3& position : c_light_positions) {
            graphics::draw(
                sphere_mesh_,
                {.scale = Vec3{0.5f}, .position = position},
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

        camera_.set_clear_flags(CameraClearFlags::Nothing);
        camera_.render_to(output_render_);
        camera_.set_clear_flags(CameraClearFlags::Default);
    }

    ResourceLoader loader_ = App::resource_loader();

    Texture2D texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    std::array<IBLSpecularObjectTextures, 5> object_textures_ = std::to_array<IBLSpecularObjectTextures>({
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_learnopengl/textures/pbr/rusted_iron")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_learnopengl/textures/pbr/gold")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_learnopengl/textures/pbr/grass")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_learnopengl/textures/pbr/plastic")},
        IBLSpecularObjectTextures{loader_.with_prefix("oscar_learnopengl/textures/pbr/wall")},
    });

    RenderTexture projected_map_ = load_equirectangular_hdr_texture_into_cubemap(loader_);
    RenderTexture irradiance_map_ = create_irradiance_cubemap(loader_, projected_map_);
    Cubemap prefilter_map_ = create_prefiltered_environment_map(loader_, projected_map_);
    Texture2D brdf_lookup_ = create_2D_brdf_lookup(loader_);
    RenderTexture output_render_;

    Material background_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.vert"),
        loader_.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{2.0f, 2.0f, 2.0f};
    Material pbr_material_ = create_material(loader_);
    Mesh sphere_mesh_ = SphereGeometry{1.0f, 64, 64};

    MouseCapturingCamera camera_ = create_camera();

    PerfPanel perf_panel_{"Perf"};
};


CStringView osc::LOGLPBRSpecularIrradianceTexturedTab::id()
{
    return c_tab_string_id;
}

osc::LOGLPBRSpecularIrradianceTexturedTab::LOGLPBRSpecularIrradianceTexturedTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLPBRSpecularIrradianceTexturedTab::LOGLPBRSpecularIrradianceTexturedTab(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTexturedTab& osc::LOGLPBRSpecularIrradianceTexturedTab::operator=(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTexturedTab::~LOGLPBRSpecularIrradianceTexturedTab() noexcept = default;

UID osc::LOGLPBRSpecularIrradianceTexturedTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLPBRSpecularIrradianceTexturedTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::impl_on_draw()
{
    impl_->on_draw();
}
