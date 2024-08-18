#include "LOGLDeferredShadingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/DeferredShading";

    constexpr auto c_object_positions = std::to_array<Vec3>({
        {-3.0,  -0.5, -3.0},
        { 0.0,  -0.5, -3.0},
        { 3.0,  -0.5, -3.0},
        {-3.0,  -0.5,  0.0},
        { 0.0,  -0.5,  0.0},
        { 3.0,  -0.5,  0.0},
        {-3.0,  -0.5,  3.0},
        { 0.0,  -0.5,  3.0},
        { 3.0,  -0.5,  3.0},
    });
    constexpr size_t c_num_lights = 32;

    Vec3 generate_scene_light_position(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{-3.0f, 3.0f};
        return {dist(rng), dist(rng), dist(rng)};
    }

    Color generate_scene_light_color(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{0.5f, 1.0f};
        return {dist(rng), dist(rng), dist(rng), 1.0f};
    }

    std::vector<Vec3> generate_n_scene_light_positions(size_t n)
    {
        const auto generator = [rng = std::default_random_engine{std::random_device{}()}]() mutable
        {
            return generate_scene_light_position(rng);
        };

        std::vector<Vec3> rv;
        rv.reserve(n);
        std::generate_n(std::back_inserter(rv), n, generator);
        return rv;
    }

    std::vector<Vec3> generate_n_scene_light_colors(size_t n)
    {
        const auto generator = [rng = std::default_random_engine{std::random_device{}()}]() mutable
        {
            const Color srgb_color = generate_scene_light_color(rng);
            const Color linear_color = to_linear_colorspace(srgb_color);
            return Vec3{linear_color.r, linear_color.g, linear_color.b};
        };

        std::vector<Vec3> rv;
        rv.reserve(n);
        std::generate_n(std::back_inserter(rv), n, generator);
        return rv;
    }

    Material load_gbuffer_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/GBuffer.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/GBuffer.frag"),
        }};
    }

    RenderTexture render_texture_with_color_format(RenderTextureFormat color_format)
    {
        RenderTexture rv;
        rv.set_color_format(color_format);
        return rv;
    }

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.5f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color(Color::black());
        return rv;
    }

    struct GBufferRenderingState final {

        explicit GBufferRenderingState(IResourceLoader& loader) :
            material{load_gbuffer_material(loader)}
        {}

        Material material;
        RenderTexture albedo = render_texture_with_color_format(RenderTextureFormat::ARGB32);
        RenderTexture normal = render_texture_with_color_format(RenderTextureFormat::ARGBFloat16);
        RenderTexture position = render_texture_with_color_format(RenderTextureFormat::ARGBFloat16);
        RenderTarget render_target{
            {
                RenderTargetColorAttachment{
                    albedo.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    normal.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    position.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
            },

            RenderTargetDepthAttachment{
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
    };

    struct LightPassState final {

        explicit LightPassState(IResourceLoader& loader) :
            material{Shader{
                loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightingPass.vert"),
                loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightingPass.frag"),
            }}
        {}

        Material material;
    };
}

class osc::LOGLDeferredShadingTab::Impl final : public StandardTabImpl {
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
        draw_3d_scene();
    }

    void draw_3d_scene()
    {
        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_screenspace_rect);
        const AntiAliasingLevel aa_level = App::get().anti_aliasing_level();

        // ensure textures/buffers have correct dimensions
        {
            gbuffer_.reformat(viewport_dimensions, aa_level);
            output_texture_.set_dimensions(viewport_dimensions);
            output_texture_.set_anti_aliasing_level(aa_level);
        }

        render_3d_scene_to_gbuffers();
        render_lighting_pass();
        render_light_cubes();
        graphics::blit_to_screen(output_texture_, viewport_screenspace_rect);
        draw_gbuffer_overlays(viewport_screenspace_rect);
    }

    void render_3d_scene_to_gbuffers()
    {
        gbuffer_.material.set("uDiffuseMap", diffuse_map_);
        gbuffer_.material.set("uSpecularMap", specular_map_);

        // render scene cubes
        for (const Vec3& object_position : c_object_positions) {
            graphics::draw(
                cube_mesh_,
                {.scale = Vec3{0.5f}, .position = object_position},
                gbuffer_.material,
                camera_
            );
        }
        camera_.render_to(gbuffer_.render_target);
    }

    void draw_gbuffer_overlays(const Rect& viewport_screenspace_rect) const
    {
        constexpr float overlay_size = 200.0f;
        const Vec2 viewport_topleft = top_left_rh(viewport_screenspace_rect);
        const Vec2 overlays_bottomleft = viewport_topleft - Vec2{0.0f, overlay_size};

        graphics::blit_to_screen(
            gbuffer_.albedo,
            Rect{overlays_bottomleft + Vec2{0.0f*overlay_size, 0.0f}, overlays_bottomleft + Vec2{0.0f*overlay_size, 0.0f} + overlay_size}
        );
        graphics::blit_to_screen(
            gbuffer_.normal,
            Rect{overlays_bottomleft + Vec2{1.0f*overlay_size, 0.0f}, overlays_bottomleft + Vec2{1.0f*overlay_size, 0.0f} + overlay_size}
        );
        graphics::blit_to_screen(
            gbuffer_.position,
            Rect{overlays_bottomleft + Vec2{2.0f*overlay_size, 0.0f}, overlays_bottomleft + Vec2{2.0f*overlay_size, 0.0f} + overlay_size}
        );
    }

    void render_lighting_pass()
    {
        light_pass_.material.set("uPositionTex", gbuffer_.position);
        light_pass_.material.set("uNormalTex", gbuffer_.normal);
        light_pass_.material.set("uAlbedoTex", gbuffer_.albedo);
        light_pass_.material.set_array("uLightPositions", light_positions_);
        light_pass_.material.set_array("uLightColors", light_colors_);
        light_pass_.material.set("uLightLinear", 0.7f);
        light_pass_.material.set("uLightQuadratic", 1.8f);
        light_pass_.material.set("uViewPos", camera_.position());

        graphics::draw(quad_mesh_, identity<Transform>(), light_pass_.material, camera_);

        camera_.render_to(output_texture_);

        light_pass_.material.unset("uPositionTex");
        light_pass_.material.unset("uNormalTex");
        light_pass_.material.unset("uAlbedoTex");
    }

    void render_light_cubes()
    {
        OSC_ASSERT(light_positions_.size() == light_colors_.size());

        for (size_t i = 0; i < light_positions_.size(); ++i) {
            light_box_material_.set("uLightColor", light_colors_[i]);
            graphics::draw(cube_mesh_, {.scale = Vec3{0.125f}, .position = light_positions_[i]}, light_box_material_, camera_);
        }

        RenderTarget render_target{
            {
                RenderTargetColorAttachment{
                    output_texture_.upd_color_buffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
            },
            RenderTargetDepthAttachment{
                gbuffer_.albedo.upd_depth_buffer(),
                RenderBufferLoadAction::Load,
                RenderBufferStoreAction::DontCare,
            },
        };
        camera_.render_to(render_target);
    }

    ResourceLoader loader_ = App::resource_loader();

    // scene state
    std::vector<Vec3> light_positions_ = generate_n_scene_light_positions(c_num_lights);
    std::vector<Vec3> light_colors_ = generate_n_scene_light_colors(c_num_lights);
    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
    Mesh cube_mesh_ = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh quad_mesh_ = PlaneGeometry{2.0f, 2.0f};
    Texture2D diffuse_map_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/container2.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );
    Texture2D specular_map_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/container2_specular.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );

    // rendering state
    GBufferRenderingState gbuffer_{loader_};
    LightPassState light_pass_{loader_};

    Material light_box_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightBox.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightBox.frag"),
    }};

    RenderTexture output_texture_;
};


CStringView osc::LOGLDeferredShadingTab::id()
{
    return c_tab_string_id;
}

osc::LOGLDeferredShadingTab::LOGLDeferredShadingTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLDeferredShadingTab::LOGLDeferredShadingTab(LOGLDeferredShadingTab&&) noexcept = default;
osc::LOGLDeferredShadingTab& osc::LOGLDeferredShadingTab::operator=(LOGLDeferredShadingTab&&) noexcept = default;
osc::LOGLDeferredShadingTab::~LOGLDeferredShadingTab() noexcept = default;

UID osc::LOGLDeferredShadingTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLDeferredShadingTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLDeferredShadingTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLDeferredShadingTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLDeferredShadingTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLDeferredShadingTab::impl_on_draw()
{
    impl_->on_draw();
}
