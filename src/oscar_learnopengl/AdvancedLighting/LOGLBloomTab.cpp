#include "LOGLBloomTab.h"

#include <oscar/oscar.h>

#include <array>
#include <memory>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_scene_light_positions = std::to_array<Vec3>({
        { 0.0f, 0.5f,  1.5f},
        {-4.0f, 0.5f, -3.0f},
        { 3.0f, 0.5f,  1.0f},
        {-0.8f, 2.4f, -1.0f},
    });

    const std::array<Color, c_scene_light_positions.size()>& get_scene_light_colors()
    {
        static const auto s_scene_light_colors = std::to_array<Color>({
            to_srgb_colorspace({ 5.0f, 5.0f,  5.0f}),
            to_srgb_colorspace({10.0f, 0.0f,  0.0f}),
            to_srgb_colorspace({ 0.0f, 0.0f, 15.0f}),
            to_srgb_colorspace({ 0.0f, 5.0f,  0.0f}),
        });
        return s_scene_light_colors;
    }

    std::vector<Mat4> create_cube_transforms()
    {
        std::vector<Mat4> rv;
        rv.reserve(6);

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(0.0f, 1.5f, 0.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(2.0f, 0.0f, 1.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-1.0f, -1.0f, 2.0));
            m = rotate(m, 60_deg, UnitVec3{1.0, 0.0, 1.0});
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(0.0f, 2.7f, 4.0));
            m = rotate(m, 23_deg, UnitVec3{1.0, 0.0, 1.0});
            m = scale(m, Vec3(1.25));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-2.0f, 1.0f, -3.0));
            m = rotate(m, 124_deg, UnitVec3{1.0, 0.0, 1.0});
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-3.0f, 0.0f, 0.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        return rv;
    }

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.5f, 5.0f});
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color(Color::black());
        return rv;
    }
}

class osc::LOGLBloomTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "LearnOpenGL/Bloom"; }

    Impl() : TabPrivate{static_label()}
    {
        scene_material_.set_array("uLightPositions", c_scene_light_positions);
        scene_material_.set_array("uLightColors", get_scene_light_colors());
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
        draw_3d_scene();
    }

private:
    void draw_3d_scene()
    {
        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_screenspace_rect);

        reformat_all_textures(viewport_dimensions);
        render_scene_mrt();
        render_blurred_brightness();
        render_combined_scene(viewport_screenspace_rect);
        draw_overlays(viewport_screenspace_rect);
    }

    void reformat_all_textures(const Vec2& viewport_dimensions)
    {
        const AntiAliasingLevel aa_level = App::get().anti_aliasing_level();

        RenderTextureParams params = {
            .dimensions = viewport_dimensions,
            .anti_aliasing_level = aa_level,
            .color_format = ColorRenderBufferFormat::DefaultHDR,
        };

        // direct render targets are multisampled HDR textures
        scene_hdr_color_output_.reformat(params);
        scene_hdr_thresholded_output_.reformat(params);

        // intermediate buffers are single-sampled HDR textures
        params.anti_aliasing_level = AntiAliasingLevel::none();
        for (RenderTexture& ping_pong_buffer : ping_pong_blur_output_buffers_) {
            ping_pong_buffer.reformat(params);
        }
    }

    void render_scene_mrt()
    {
        draw_scene_cubes_to_camera();
        draw_lightboxes_to_camera();
        flush_camera_render_queue_to_mrt();
    }

    void draw_scene_cubes_to_camera()
    {
        scene_material_.set("uViewWorldPos", camera_.position());

        // draw floor
        {
            Mat4 floor_mat4 = identity<Mat4>();
            floor_mat4 = translate(floor_mat4, Vec3(0.0f, -1.0f, 0.0));
            floor_mat4 = scale(floor_mat4, Vec3(12.5f, 0.5f, 12.5f));

            MaterialPropertyBlock floor_props;
            floor_props.set("uDiffuseTexture", wood_texture_);

            graphics::draw(
                cube_mesh_,
                floor_mat4,
                scene_material_,
                camera_,
                floor_props
            );
        }

        MaterialPropertyBlock cube_props;
        cube_props.set("uDiffuseTexture", container_texture_);
        for (const auto& cube_transform : create_cube_transforms()) {
            graphics::draw(
                cube_mesh_,
                cube_transform,
                scene_material_,
                camera_,
                cube_props
            );
        }
    }

    void draw_lightboxes_to_camera()
    {
        const auto& scene_light_colors = get_scene_light_colors();

        for (size_t i = 0; i < c_scene_light_positions.size(); ++i) {
            Mat4 light_mat4 = identity<Mat4>();
            light_mat4 = translate(light_mat4, Vec3(c_scene_light_positions[i]));
            light_mat4 = scale(light_mat4, Vec3(0.25f));

            MaterialPropertyBlock light_props;
            light_props.set("uLightColor", scene_light_colors[i]);

            graphics::draw(
                cube_mesh_,
                light_mat4,
                lightbox_material_,
                camera_,
                light_props
            );
        }
    }

    void flush_camera_render_queue_to_mrt()
    {
        RenderTarget mrt{
            RenderTargetColorAttachment{
                scene_hdr_color_output_.upd_color_buffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::Resolve,
                Color::clear(),
            },
            RenderTargetColorAttachment{
                scene_hdr_thresholded_output_.upd_color_buffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::Resolve,
                Color::clear(),
            },
            RenderTargetDepthStencilAttachment{
                scene_hdr_thresholded_output_.upd_depth_buffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };
        camera_.render_to(mrt);
    }

    void render_blurred_brightness()
    {
        blur_material_.set("uInputImage", scene_hdr_thresholded_output_);

        bool horizontal = false;
        for (RenderTexture& ping_pong_buffer : ping_pong_blur_output_buffers_) {
            blur_material_.set("uHorizontal", horizontal);
            Camera camera;
            graphics::draw(quad_mesh_, identity<Transform>(), blur_material_, camera);
            camera.render_to(ping_pong_buffer);
            blur_material_.unset("uInputImage");

            horizontal = !horizontal;
        }
    }

    void render_combined_scene(const Rect& viewport_rect)
    {
        final_compositing_material_.set("uHDRSceneRender", scene_hdr_color_output_);
        final_compositing_material_.set("uBloomBlur", ping_pong_blur_output_buffers_[0]);
        final_compositing_material_.set("uBloom", true);
        final_compositing_material_.set("uExposure", 1.0f);

        Camera camera;
        graphics::draw(quad_mesh_, identity<Transform>(), final_compositing_material_, camera);
        camera.set_pixel_rect(viewport_rect);
        camera.render_to_screen();

        final_compositing_material_.unset("uBloomBlur");
        final_compositing_material_.unset("uHDRSceneRender");
    }

    void draw_overlays(const Rect& viewport_screenspace_rect)
    {
        constexpr float w = 200.0f;

        const auto texture_ptrs = std::to_array<const RenderTexture*>({
            &scene_hdr_color_output_,
            &scene_hdr_thresholded_output_,
            ping_pong_blur_output_buffers_.data(),
            ping_pong_blur_output_buffers_.data() + 1,
        });

        for (size_t i = 0; i < texture_ptrs.size(); ++i) {
            const Vec2 offset = {static_cast<float>(i)*w, 0.0f};
            const Rect overlay_rect{
                viewport_screenspace_rect.p1 + offset,
                viewport_screenspace_rect.p1 + offset + w,
            };

            graphics::blit_to_screen(*texture_ptrs[i], overlay_rect);
        }
    }

    ResourceLoader loader_ = App::resource_loader();

    Material scene_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.frag"),
    }};

    Material lightbox_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.frag"),
    }};

    Material blur_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.frag"),
    }};

    Material final_compositing_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.frag"),
    }};

    Texture2D wood_texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Texture2D container_texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/container2.png"),
        ColorSpace::sRGB
    );
    Mesh cube_mesh_ = BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}};
    Mesh quad_mesh_ = PlaneGeometry{{.width = 2.0f, .height = 2.0f}};

    RenderTexture scene_hdr_color_output_;
    RenderTexture scene_hdr_thresholded_output_;
    std::array<RenderTexture, 2> ping_pong_blur_output_buffers_;

    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
};


CStringView osc::LOGLBloomTab::id() { return Impl::static_label(); }

osc::LOGLBloomTab::LOGLBloomTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}

void osc::LOGLBloomTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLBloomTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLBloomTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLBloomTab::impl_on_draw() { private_data().on_draw(); }
