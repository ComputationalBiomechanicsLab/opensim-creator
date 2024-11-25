#include "LOGLSSAOTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 50.0f});
        rv.set_background_color(Color::black());
        return rv;
    }

    std::vector<Vec3> generate_sample_kernel(size_t num_samples)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> zero_to_one{0.0f, 1.0f};
        std::uniform_real_distribution<float> minus_one_to_one{-1.0f, 1.0f};

        std::vector<Vec3> rv;
        rv.reserve(num_samples);
        for (size_t i = 0; i < num_samples; ++i) {
            // scale such that they are more aligned to the center of the kernel
            float scale = static_cast<float>(i)/static_cast<float>(num_samples);
            scale = lerp(0.1f, 1.0f, scale*scale);

            Vec3 sample = {minus_one_to_one(rng), minus_one_to_one(rng), minus_one_to_one(rng)};
            sample = normalize(sample);
            sample *= zero_to_one(rng);
            sample *= scale;

            rv.push_back(sample);
        }

        return rv;
    }

    std::vector<Color> generate_noise_texture_pixels(size_t num_pixels) {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> minus_one_to_one{-1.0f, 1.0f};

        std::vector<Color> rv;
        rv.reserve(num_pixels);
        for (size_t i = 0; i < num_pixels; ++i) {
            rv.emplace_back(
                minus_one_to_one(rng),
                minus_one_to_one(rng),
                0.0f,  // rotate around z-axis in tangent space
                0.0f   // ignored (Texture2D doesn't support RGB --> RGBA upload conversion)
            );
        }
        return rv;
    }

    Texture2D generate_noise_texture(Vec2i dimensions)
    {
        const std::vector<Color> pixels =
            generate_noise_texture_pixels(static_cast<size_t>(area_of(dimensions)));

        Texture2D rv{
            dimensions,
            TextureFormat::RGBAFloat,
            ColorSpace::Linear,
            TextureWrapMode::Repeat,
            TextureFilterMode::Linear,
        };
        rv.set_pixel_data(view_object_representations<uint8_t>(pixels));
        return rv;
    }

    Material load_gbuffer_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Geometry.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Geometry.frag"),
        }};
    }

    RenderTexture render_texture_with_color_format(ColorRenderBufferFormat format)
    {
        RenderTexture rv;
        rv.set_color_format(format);
        return rv;
    }

    Material load_ssao_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/SSAO.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/SSAO.frag"),
        }};
    }

    Material load_blur_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Blur.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Blur.frag"),
        }};
    }

    Material load_lighting_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Lighting.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/ssao/Lighting.frag"),
        }};
    }
}

class osc::LOGLSSAOTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/SSAO"; }

    explicit Impl(LOGLSSAOTab& owner, Widget& parent) :
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
        draw_3d_scene();
        perf_panel_.on_draw();
    }

private:
    void draw_3d_scene()
    {
        const Rect viewport_screen_space_rect = ui::get_main_viewport_workspace_screenspace_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_screen_space_rect);

        // ensure textures/buffers have correct dimensions
        {
            constexpr AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel::none();

            gbuffer_state_.reformat(viewport_dimensions, anti_aliasing_level);
            ssao_state_.reformat(viewport_dimensions, anti_aliasing_level);
            blur_state_.reformat(viewport_dimensions, anti_aliasing_level);
            lighting_state_.reformat(viewport_dimensions, anti_aliasing_level);
        }

        render_geometry_pass_to_gbuffers();
        render_ssao_pass(viewport_dimensions);
        render_blur_pass();
        render_lighting_pass();
        graphics::blit_to_screen(lighting_state_.output_texture, viewport_screen_space_rect);
        draw_debug_overlays(viewport_screen_space_rect);
    }

    void render_geometry_pass_to_gbuffers()
    {
        // render cube
        {
            gbuffer_state_.material.set("uInvertedNormals", true);
            graphics::draw(
                cube_mesh_,
                {.scale = Vec3{7.5f}, .position = {0.0f, 7.0f, 0.0f}},
                gbuffer_state_.material,
                camera_
            );
        }

        // render sphere
        {
            gbuffer_state_.material.set("uInvertedNormals", false);
            graphics::draw(
                sphere_mesh_,
                {.position = {0.0f, 0.5f, 0.0f}},
                gbuffer_state_.material,
                camera_
            );
        }

        camera_.render_to(gbuffer_state_.render_target);
    }

    void render_ssao_pass(const Vec2& viewport_dimensions)
    {
        ssao_state_.material.set("uPositionTex", gbuffer_state_.position);
        ssao_state_.material.set("uNormalTex", gbuffer_state_.normal);
        ssao_state_.material.set("uNoiseTex", noise_texture_);
        ssao_state_.material.set_array("uSamples", sample_kernel_);
        ssao_state_.material.set("uNoiseScale", viewport_dimensions / Vec2{noise_texture_.dimensions()});
        ssao_state_.material.set("uKernelSize", static_cast<int32_t>(sample_kernel_.size()));
        ssao_state_.material.set("uRadius", 0.5f);
        ssao_state_.material.set("uBias", 0.125f);

        graphics::draw(quad_mesh_, identity<Transform>(), ssao_state_.material, camera_);
        camera_.render_to(ssao_state_.output_texture);

        ssao_state_.material.unset("uPositionTex");
        ssao_state_.material.unset("uNormalTex");
    }

    void render_blur_pass()
    {
        blur_state_.material.set("uSSAOTex", ssao_state_.output_texture);

        graphics::draw(quad_mesh_, identity<Transform>(), blur_state_.material, camera_);
        camera_.render_to(blur_state_.output_texture);

        blur_state_.material.unset("uSSAOTex");
    }

    void render_lighting_pass()
    {
        lighting_state_.material.set("uPositionTex", gbuffer_state_.position);
        lighting_state_.material.set("uNormalTex", gbuffer_state_.normal);
        lighting_state_.material.set("uAlbedoTex", gbuffer_state_.albedo);
        lighting_state_.material.set("uSSAOTex", ssao_state_.output_texture);
        lighting_state_.material.set("uLightPosition", light_position_);
        lighting_state_.material.set("uLightColor", light_color_);
        lighting_state_.material.set("uLightLinear", 0.09f);
        lighting_state_.material.set("uLightQuadratic", 0.032f);

        graphics::draw(quad_mesh_, identity<Transform>(), lighting_state_.material, camera_);
        camera_.render_to(lighting_state_.output_texture);

        lighting_state_.material.unset("uPositionTex");
        lighting_state_.material.unset("uNormalTex");
        lighting_state_.material.unset("uAlbedoTex");
        lighting_state_.material.unset("uSSAOTex");
    }

    void draw_debug_overlays(const Rect& viewport_screen_space_rect)
    {
        const float overlay_size = 200.0f;

        const auto textures = std::to_array<const RenderTexture*>({
            &gbuffer_state_.albedo,
            &gbuffer_state_.normal,
            &gbuffer_state_.position,
            &ssao_state_.output_texture,
            &blur_state_.output_texture,
        });

        const Vec2 viewport_top_left = top_left_rh(viewport_screen_space_rect);
        for (size_t i = 0; i < textures.size(); ++i) {
            const float offset = static_cast<float>(i)*overlay_size;
            const Vec2 overlay_bottom_left = {viewport_top_left.x + offset, viewport_top_left.y - overlay_size};
            const Vec2 overlay_top_right = overlay_bottom_left + Vec2{overlay_size};
            graphics::blit_to_screen(*textures[i], Rect{overlay_bottom_left, overlay_top_right});
        }
    }

    std::vector<Vec3> sample_kernel_ = generate_sample_kernel(64);
    Texture2D noise_texture_ = generate_noise_texture({4, 4});
    Vec3 light_position_ = {2.0f, 4.0f, -2.0f};
    Color light_color_ = {0.2f, 0.2f, 0.7f, 1.0f};

    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();

    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 32, .num_height_segments = 32}};
    Mesh cube_mesh_ = BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}};
    Mesh quad_mesh_ = PlaneGeometry{{.width = 2.0f, .height = 2.0f}};

    // rendering state
    struct GBufferRenderingState final {
        Material material = load_gbuffer_material(App::resource_loader());
        RenderTexture albedo = render_texture_with_color_format(ColorRenderBufferFormat::R8G8B8A8_SRGB);
        RenderTexture normal = render_texture_with_color_format(ColorRenderBufferFormat::R16G16B16_SFLOAT);
        RenderTexture position = render_texture_with_color_format(ColorRenderBufferFormat::R16G16B16_SFLOAT);
        RenderTarget render_target{
            RenderTargetColorAttachment{
                albedo.upd_color_buffer(),
                RenderBufferLoadAction::Load,
                RenderBufferStoreAction::Resolve,
                Color::black(),
            },
            RenderTargetColorAttachment{
                normal.upd_color_buffer(),
                RenderBufferLoadAction::Load,
                RenderBufferStoreAction::Resolve,
                Color::black(),
            },
            RenderTargetColorAttachment{
                position.upd_color_buffer(),
                RenderBufferLoadAction::Load,
                RenderBufferStoreAction::Resolve,
                Color::black(),
            },
            RenderTargetDepthStencilAttachment{
                albedo.upd_depth_buffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };

        void reformat(Vec2 dimensions, AntiAliasingLevel aa_level)
        {
            for (RenderTexture* texture_ptr : {&albedo, &normal, &position}) {
                texture_ptr->reformat({
                    .dimensions = dimensions,
                    .anti_aliasing_level = aa_level,
                    .color_format = texture_ptr->color_format(),
                });
            }
        }
    } gbuffer_state_;

    struct SSAORenderingState final {
        Material material = load_ssao_material(App::resource_loader());
        RenderTexture output_texture = render_texture_with_color_format(ColorRenderBufferFormat::R8_UNORM);

        void reformat(Vec2 dimensions, AntiAliasingLevel aa_level)
        {
            output_texture.set_dimensions(dimensions);
            output_texture.set_anti_aliasing_level(aa_level);
        }
    } ssao_state_;

    struct BlurRenderingState final {
        Material material = load_blur_material(App::resource_loader());
        RenderTexture output_texture = render_texture_with_color_format(ColorRenderBufferFormat::R8_UNORM);

        void reformat(Vec2 dimensions, AntiAliasingLevel aa_level)
        {
            output_texture.set_dimensions(dimensions);
            output_texture.set_anti_aliasing_level(aa_level);
        }
    } blur_state_;

    struct LightingRenderingState final {
        Material material = load_lighting_material(App::resource_loader());
        RenderTexture output_texture = render_texture_with_color_format(ColorRenderBufferFormat::R8G8B8A8_SRGB);

        void reformat(Vec2 dimensions, AntiAliasingLevel aa_level)
        {
            output_texture.set_dimensions(dimensions);
            output_texture.set_anti_aliasing_level(aa_level);
        }
    } lighting_state_;

    PerfPanel perf_panel_;
};


CStringView osc::LOGLSSAOTab::id() { return Impl::static_label(); }
osc::LOGLSSAOTab::LOGLSSAOTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLSSAOTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLSSAOTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLSSAOTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLSSAOTab::impl_on_draw() { private_data().on_draw(); }
