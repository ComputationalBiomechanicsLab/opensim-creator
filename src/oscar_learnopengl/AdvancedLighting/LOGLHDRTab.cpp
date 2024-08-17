#include "LOGLHDRTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_light_positions = std::to_array<Vec3>({
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    });

    constexpr CStringView c_tab_string_id = "LearnOpenGL/HDR";

    std::array<Color, c_light_positions.size()> GetLightColors()
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
        return {.scale = {2.5f, 2.5f, 27.5f}, .position = {0.0f, 0.0f, 25.0f}};
    }

    MouseCapturingCamera create_scene_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        rv.eulers() = {0_deg, 180_deg, 0_deg};
        return rv;
    }

    Material create_scene_material(IResourceLoader& loader)
    {
        Texture2D wood_texture = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/wood.png"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.frag"),
        }};
        rv.set_array<Vec3>("uSceneLightPositions", c_light_positions);
        rv.set_array<Color>("uSceneLightColors", GetLightColors());
        rv.set_texture("uDiffuseTexture", wood_texture);
        rv.set_bool("uInverseNormals", true);
        return rv;
    }

    Material create_tonemap_material(IResourceLoader& loader)
    {
        return Material{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.frag"),
        }};
    }
}

class osc::LOGLHDRTab::Impl final : public StandardTabImpl {
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
        draw_3d_scene_to_hdr_texture();
        draw_hdr_texture_via_tonemapper_to_screen();
        draw_2d_ui();
    }

    void draw_3d_scene_to_hdr_texture()
    {
        // reformat intermediate HDR texture to match tab dimensions etc.
        {
            RenderTextureParams params = {
                .dimensions = ui::get_main_viewport_workspace_screen_dimensions(),
                .anti_aliasing_level = App::get().anti_aliasing_level(),
            };
            if (use_16bit_format_) {
                params.color_format = RenderTextureFormat::ARGBFloat16;
            }

            scene_hdr_texture_.reformat(params);
        }

        graphics::draw(cube_mesh_, corridoor_transform_, scene_material_, camera_);
        camera_.render_to(scene_hdr_texture_);
    }

    void draw_hdr_texture_via_tonemapper_to_screen()
    {
        Camera ortho_camera;
        ortho_camera.set_background_color(Color::clear());
        ortho_camera.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        ortho_camera.set_projection_matrix_override(identity<Mat4>());
        ortho_camera.set_view_matrix_override(identity<Mat4>());

        tonemap_material_.set_render_texture("uTexture", scene_hdr_texture_);
        tonemap_material_.set_bool("uUseTonemap", use_tonemap_);
        tonemap_material_.set<float>("uExposure", exposure_);

        graphics::draw(quad_mesh_, identity<Transform>(), tonemap_material_, ortho_camera);
        ortho_camera.render_to_screen();

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
    Mesh cube_mesh_ = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh quad_mesh_ = PlaneGeometry{2.0f, 2.0f};
    Transform corridoor_transform_ = calc_corridoor_transform();
    RenderTexture scene_hdr_texture_;
    float exposure_ = 1.0f;
    bool use_16bit_format_ = true;
    bool use_tonemap_ = true;
};


CStringView osc::LOGLHDRTab::id()
{
    return c_tab_string_id;
}

osc::LOGLHDRTab::LOGLHDRTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLHDRTab::LOGLHDRTab(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab& osc::LOGLHDRTab::operator=(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab::~LOGLHDRTab() noexcept = default;

UID osc::LOGLHDRTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLHDRTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLHDRTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLHDRTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLHDRTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLHDRTab::impl_on_draw()
{
    impl_->on_draw();
}
