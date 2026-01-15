#include "LOGLBloomTab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/graphics/MaterialPropertyBlock.h>
#include <liboscar/graphics/RenderTarget.h>
#include <liboscar/graphics/RenderTargetColorAttachment.h>
#include <liboscar/graphics/RenderTargetDepthStencilAttachment.h>
#include <liboscar/graphics/RenderTexture.h>
#include <liboscar/graphics/RenderTextureParams.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/PlaneGeometry.h>
#include <liboscar/maths/Matrix4x4.h>
#include <liboscar/maths/MatrixFunctions.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/ui/mouse_capturing_camera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <array>
#include <memory>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_scene_light_positions = std::to_array<Vector3>({
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

    std::vector<Matrix4x4> create_cube_transforms()
    {
        std::vector<Matrix4x4> rv;
        rv.reserve(6);

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(0.0f, 1.5f, 0.0));
            m = scale(m, Vector3(0.5f));
            rv.push_back(m);
        }

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(2.0f, 0.0f, 1.0));
            m = scale(m, Vector3(0.5f));
            rv.push_back(m);
        }

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(-1.0f, -1.0f, 2.0));
            m = rotate(m, 60_deg, normalize(Vector3{1.0, 0.0, 1.0}));
            rv.push_back(m);
        }

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(0.0f, 2.7f, 4.0));
            m = rotate(m, 23_deg, normalize(Vector3{1.0, 0.0, 1.0}));
            m = scale(m, Vector3(1.25));
            rv.push_back(m);
        }

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(-2.0f, 1.0f, -3.0));
            m = rotate(m, 124_deg, normalize(Vector3{1.0, 0.0, 1.0}));
            rv.push_back(m);
        }

        {
            Matrix4x4 m = identity<Matrix4x4>();
            m = translate(m, Vector3(-3.0f, 0.0f, 0.0));
            m = scale(m, Vector3(0.5f));
            rv.push_back(m);
        }

        return rv;
    }

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera camera;
        camera.set_position({0.0f, 0.5f, 5.0f});
        camera.set_clipping_planes({0.1f, 100.0f});
        camera.set_background_color(Color::black());
        return camera;
    }
}

class osc::LOGLBloomTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/Bloom"; }

    explicit Impl(LOGLBloomTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
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
        const Rect workspace_screen_space_rect = ui::get_main_window_workspace_screen_space_rect();
        const float device_pixel_ratio = App::get().main_window_device_pixel_ratio();
        const Vector2 workspace_pixel_dimensions = device_pixel_ratio * workspace_screen_space_rect.dimensions();

        reformat_all_textures(workspace_pixel_dimensions, device_pixel_ratio);
        render_scene_mrt();
        render_blurred_brightness();
        render_combined_scene(workspace_screen_space_rect);
        draw_overlays(workspace_screen_space_rect);
    }

    void reformat_all_textures(const Vector2& viewport_pixel_dimensions, float device_pixel_ratio)
    {
        const AntiAliasingLevel aa_level = App::get().anti_aliasing_level();

        RenderTextureParams params = {
            .pixel_dimensions = viewport_pixel_dimensions,
            .device_pixel_ratio = device_pixel_ratio,
            .anti_aliasing_level = aa_level,
            .color_format = ColorRenderBufferFormat::DefaultHDR,
        };

        // direct render targets are multi-sampled HDR textures
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
            Matrix4x4 floor_transform = identity<Matrix4x4>();
            floor_transform = translate(floor_transform, Vector3(0.0f, -1.0f, 0.0));
            floor_transform = scale(floor_transform, Vector3(12.5f, 0.5f, 12.5f));

            MaterialPropertyBlock floor_props;
            floor_props.set("uDiffuseTexture", wood_texture_);

            graphics::draw(
                cube_mesh_,
                floor_transform,
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
            Matrix4x4 light_transform = identity<Matrix4x4>();
            light_transform = translate(light_transform, Vector3(c_scene_light_positions[i]));
            light_transform = scale(light_transform, Vector3(0.25f));

            MaterialPropertyBlock light_props;
            light_props.set("uLightColor", scene_light_colors[i]);

            graphics::draw(
                cube_mesh_,
                light_transform,
                lightbox_material_,
                camera_,
                light_props
            );
        }
    }

    void flush_camera_render_queue_to_mrt()
    {
        const RenderTarget mrt{
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

    void render_combined_scene(const Rect& viewport_screen_space_rect)
    {
        final_compositing_material_.set("uHDRSceneRender", scene_hdr_color_output_);
        final_compositing_material_.set("uBloomBlur", ping_pong_blur_output_buffers_[0]);
        final_compositing_material_.set("uBloom", true);
        final_compositing_material_.set("uExposure", 1.0f);

        Camera camera;
        graphics::draw(quad_mesh_, identity<Transform>(), final_compositing_material_, camera);
        camera.set_pixel_rect(viewport_screen_space_rect);
        camera.render_to_main_window();

        final_compositing_material_.unset("uBloomBlur");
        final_compositing_material_.unset("uHDRSceneRender");
    }

    void draw_overlays(const Rect& viewport_screen_space_rect)
    {
        constexpr float overlay_width = 200.0f;

        const auto texture_pointers = std::to_array<const RenderTexture*>({
            &scene_hdr_color_output_,
            &scene_hdr_thresholded_output_,
            ping_pong_blur_output_buffers_.data(),
            ping_pong_blur_output_buffers_.data() + 1,
        });

        for (size_t i = 0; i < texture_pointers.size(); ++i) {
            const Vector2 offset = {static_cast<float>(i)*overlay_width, 0.0f};
            const Rect overlay_rect = Rect::from_corners(
                viewport_screen_space_rect.ypu_bottom_left() + offset,
                viewport_screen_space_rect.ypu_bottom_left() + offset + overlay_width
            );

            graphics::blit_to_main_window(*texture_pointers[i], overlay_rect);
        }
    }

    ResourceLoader loader_ = App::resource_loader();

    Material scene_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Bloom.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Bloom.frag"),
    }};

    Material lightbox_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/LightBox.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/LightBox.frag"),
    }};

    Material blur_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Blur.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Blur.frag"),
    }};

    Material final_compositing_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Final.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/bloom/Final.frag"),
    }};

    Texture2D wood_texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/wood.jpg"),
        ColorSpace::sRGB
    );
    Texture2D container_texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/container2.jpg"),
        ColorSpace::sRGB
    );
    Mesh cube_mesh_ = BoxGeometry{{.dimensions = Vector3{2.0f}}};
    Mesh quad_mesh_ = PlaneGeometry{{.dimensions = Vector2{2.0f}}};

    RenderTexture scene_hdr_color_output_;
    RenderTexture scene_hdr_thresholded_output_;
    std::array<RenderTexture, 2> ping_pong_blur_output_buffers_;

    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
};


CStringView osc::LOGLBloomTab::id() { return Impl::static_label(); }

osc::LOGLBloomTab::LOGLBloomTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLBloomTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLBloomTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLBloomTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLBloomTab::impl_on_draw() { private_data().on_draw(); }
