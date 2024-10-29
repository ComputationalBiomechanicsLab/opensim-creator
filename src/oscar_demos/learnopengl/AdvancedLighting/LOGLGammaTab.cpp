#include "LOGLGammaTab.h"

#include <oscar/oscar.h>

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
            loader.open("oscar_demos/learnopengl/textures/wood.jpg"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/Gamma.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedLighting/Gamma.frag"),
        }};
        rv.set("uFloorTexture", wood_texture);
        rv.set_array("uLightPositions", c_light_positions);
        rv.set_array("uLightColors", c_light_colors);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedLighting/Gamma"; }

    explicit Impl(LOGLGammaTab& owner, Widget& parent) :
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
        draw_2d_ui();
    }

private:
    void draw_3d_scene()
    {
        // clear screen and ensure camera has correct pixel rect
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        // render scene
        material_.set("uViewPos", camera_.position());
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


CStringView osc::LOGLGammaTab::id() { return Impl::static_label(); }

osc::LOGLGammaTab::LOGLGammaTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLGammaTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLGammaTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLGammaTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLGammaTab::impl_on_draw() { private_data().on_draw(); }
