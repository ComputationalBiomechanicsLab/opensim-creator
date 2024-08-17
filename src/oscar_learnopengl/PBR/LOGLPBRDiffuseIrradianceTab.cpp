#include "LOGLPBRDiffuseIrradianceTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

namespace graphics = osc::graphics;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/PBR/DiffuseIrradiance";

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
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture load_equirectangular_hdr_texture_into_cubemap(
        IResourceLoader& loader)
    {
        Texture2D hdr_texture = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear,
            ImageLoadingFlags::FlipVertically
        );
        hdr_texture.set_wrap_mode(TextureWrapMode::Clamp);
        hdr_texture.set_filter_mode(TextureFilterMode::Linear);

        RenderTexture cubemap_render_texture{{
            .dimensions = {512, 512},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = RenderTextureFormat::RGBFloat16,
        }};

        // create a 90 degree cube cone projection matrix
        const Mat4 projection_matrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.geom"),
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.frag"),
        }};
        material.set_texture("uEquirectangularMap", hdr_texture);
        material.set_mat4_array("uShadowMatrices", calc_cubemap_view_proj_matrices(projection_matrix, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(cubemap_render_texture);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return cubemap_render_texture;
    }

    RenderTexture create_irradiance_cubemap(
        IResourceLoader& loader,
        const RenderTexture& skybox)
    {
        RenderTexture irradiance_cubemap{{
            .dimensions = {32, 32},
            .dimensionality = TextureDimensionality::Cube,
            .color_format = RenderTextureFormat::RGBFloat16,
        }};

        const Mat4 capture_projection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.geom"),
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.frag"),
        }};
        material.set_render_texture("uEnvironmentMap", skybox);
        material.set_mat4_array("uShadowMatrices", calc_cubemap_view_proj_matrices(capture_projection, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(irradiance_cubemap);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return irradiance_cubemap;
    }

    Material CreateMaterial(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/PBR.vert"),
            loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/PBR.frag"),
        }};
        rv.set<float>("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRDiffuseIrradianceTab::Impl final : public StandardTabImpl {
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
        camera_.on_draw();
        draw_3D_render();
        draw_background();
        draw_2D_ui();
    }

    void draw_3D_render()
    {
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        pbr_material_.set<Vec3>("uCameraWorldPos", camera_.position());
        pbr_material_.set_array<Vec3>("uLightPositions", c_light_positions);
        pbr_material_.set_array<Vec3>("uLightColors", c_light_radiances);
        pbr_material_.set_render_texture("uIrradianceMap", irradiance_map_);

        draw_spheres();
        draw_lights();

        camera_.render_to_screen();
    }

    void draw_spheres()
    {
        pbr_material_.set<Vec3>("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_num_rows; ++row) {
            pbr_material_.set<float>("uMetallicity", static_cast<float>(row) / static_cast<float>(c_num_rows));

            for (int col = 0; col < c_num_cols; ++col) {
                const float normalizedCol = static_cast<float>(col) / static_cast<float>(c_num_cols);
                pbr_material_.set<float>("uRoughness", clamp(normalizedCol, 0.005f, 1.0f));

                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;

                graphics::draw(sphere_mesh_, {.position = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        pbr_material_.set<Vec3>("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (const Vec3& light_positions : c_light_positions) {
            graphics::draw(sphere_mesh_, {.scale = Vec3{0.5f}, .position = light_positions}, pbr_material_, camera_);
        }
    }

    void draw_background()
    {
        background_material_.set_render_texture("uEnvironmentMap", projected_map_);
        background_material_.set_depth_function(DepthFunction::LessOrEqual);  // for skybox depth trick
        graphics::draw(cube_mesh_, identity<Transform>(), background_material_, camera_);
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.set_clear_flags(CameraClearFlags::Nothing);
        camera_.render_to_screen();
        camera_.set_clear_flags(CameraClearFlags::Default);
    }

    void draw_2D_ui()
    {
        if (ui::begin_panel("Controls")) {
            float ao = pbr_material_.get<float>("uAO").value_or(1.0f);
            if (ui::draw_float_slider("ao", &ao, 0.0f, 1.0f)) {
                pbr_material_.set<float>("uAO", ao);
            }
        }
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();

    Texture2D texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    RenderTexture projected_map_ = load_equirectangular_hdr_texture_into_cubemap(loader_);
    RenderTexture irradiance_map_ = create_irradiance_cubemap(loader_, projected_map_);

    Material background_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Background.vert"),
        loader_.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Background.frag"),
    }};

    Mesh cube_mesh_ = BoxGeometry{2.0f, 2.0f, 2.0f};
    Material pbr_material_ = CreateMaterial(loader_);
    Mesh sphere_mesh_ = SphereGeometry{1.0f, 64, 64};
    MouseCapturingCamera camera_ = create_camera();
};


CStringView osc::LOGLPBRDiffuseIrradianceTab::id()
{
    return c_tab_string_id;
}

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab& osc::LOGLPBRDiffuseIrradianceTab::operator=(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab::~LOGLPBRDiffuseIrradianceTab() noexcept = default;

UID osc::LOGLPBRDiffuseIrradianceTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLPBRDiffuseIrradianceTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLPBRDiffuseIrradianceTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLPBRDiffuseIrradianceTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLPBRDiffuseIrradianceTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLPBRDiffuseIrradianceTab::impl_on_draw()
{
    impl_->on_draw();
}
