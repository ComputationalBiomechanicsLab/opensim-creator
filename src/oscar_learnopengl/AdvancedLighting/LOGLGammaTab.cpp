#include "LOGLGammaTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_light_positions = std::to_array<Vec3>({
        {-3.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f},
        { 3.0f, 0.0f, 0.0f},
    });

    constexpr auto c_light_colors = std::to_array<Color>({
        {0.25f, 0.25f, 0.25f, 1.0f},
        {0.50f, 0.50f, 0.50f, 1.0f},
        {0.75f, 0.75f, 0.75f, 1.0f},
        {1.00f, 1.00f, 1.00f, 1.0f},
    });

    constexpr CStringView c_tab_string_id = "LearnOpenGL/Gamma";

    Mesh generate_plane()
    {
        Mesh rv;
        rv.set_vertices({
            { 10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f, -10.0f},

            { 10.0f, -0.5f,  10.0f},
            {-10.0f, -0.5f, -10.0f},
            { 10.0f, -0.5f, -10.0f},
        });
        rv.set_tex_coords({
            {10.0f, 0.0f},
            {0.0f,  0.0f},
            {0.0f,  10.0f},

            {10.0f, 0.0f},
            {0.0f,  10.0f},
            {10.0f, 10.0f},
        });
        rv.set_normals({
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        });
        rv.set_indices({0, 2, 1, 3, 5, 4});
        return rv;
    }

    MouseCapturingCamera create_scene_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material create_floor_material(IResourceLoader& loader)
    {
        const Texture2D wood_texture = load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/wood.png"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.frag"),
        }};
        rv.set_texture("uFloorTexture", wood_texture);
        rv.set_vec3_array("uLightPositions", c_light_positions);
        rv.set_array<Color>("uLightColors", c_light_colors);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final : public StandardTabImpl {
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
        draw_2d_ui();
    }

    void draw_3d_scene()
    {
        // clear screen and ensure camera has correct pixel rect
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        // render scene
        material_.set_vec3("uViewPos", camera_.position());
        graphics::draw(plane_mesh_, identity<Transform>(), material_, camera_);
        camera_.render_to_screen();
    }

    void draw_2d_ui()
    {
        ui::begin_panel("controls");
        ui::draw_text("no need to gamma correct - OSC is a gamma-corrected renderer");
        ui::end_panel();
    }

    Material material_ = create_floor_material(App::resource_loader());
    Mesh plane_mesh_ = generate_plane();
    MouseCapturingCamera camera_ = create_scene_camera();
};


CStringView osc::LOGLGammaTab::id()
{
    return c_tab_string_id;
}

osc::LOGLGammaTab::LOGLGammaTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLGammaTab::LOGLGammaTab(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab& osc::LOGLGammaTab::operator=(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab::~LOGLGammaTab() noexcept = default;

UID osc::LOGLGammaTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLGammaTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLGammaTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLGammaTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLGammaTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLGammaTab::impl_on_draw()
{
    impl_->on_draw();
}
