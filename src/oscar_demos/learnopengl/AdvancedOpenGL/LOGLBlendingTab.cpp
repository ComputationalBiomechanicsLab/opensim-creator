#include "LOGLBlendingTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_window_locations = std::to_array<Vec3>({
        {-1.5f, 0.0f, -0.48f},
        { 1.5f, 0.0f,  0.51f},
        { 0.0f, 0.0f,  0.7f},
        {-0.3f, 0.0f, -2.3f},
        { 0.5f, 0.0f, -0.6},
    });

    Mesh generate_plane()
    {
        Mesh rv;
        rv.set_vertices({
            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},

            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},
            { 5.0f, -0.5f, -5.0f},
        });
        rv.set_tex_coords({
            {2.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 2.0f},

            {2.0f, 0.0f},
            {0.0f, 2.0f},
            {2.0f, 2.0f},
        });
        rv.set_indices({0, 2, 1, 3, 5, 4});
        return rv;
    }

    Mesh generate_transparent()
    {
        Mesh rv;
        rv.set_vertices({
            {0.0f,  0.5f, 0.0f},
            {0.0f, -0.5f, 0.0f},
            {1.0f, -0.5f, 0.0f},

            {0.0f,  0.5f, 0.0f},
            {1.0f, -0.5f, 0.0f},
            {1.0f,  0.5f, 0.0f},
        });
        rv.set_tex_coords({
            {0.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 1.0f},

            {0.0f, 0.0f},
            {1.0f, 1.0f},
            {1.0f, 0.0f},
        });
        rv.set_indices({0, 1, 2, 3, 4, 5});
        return rv;
    }

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLBlendingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedOpenGL/Blending"; }

    explicit Impl(LOGLBlendingTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {
        blending_material_.set_transparent(true);
        log_viewer_.open();
        perf_panel_.open();
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

        // clear screen and ensure camera has correct pixel rect
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        // cubes
        {
            opaque_material_.set("uTexture", marble_texture_);
            graphics::draw(cube_mesh_, {.position = {-1.0f, 0.0f, -1.0f}}, opaque_material_, camera_);
            graphics::draw(cube_mesh_, {.position = { 1.0f, 0.0f, -1.0f}}, opaque_material_, camera_);
        }

        // floor
        {
            opaque_material_.set("uTexture", metal_texture_);
            graphics::draw(plane_mesh_, identity<Transform>(), opaque_material_, camera_);
        }

        // windows
        {
            blending_material_.set("uTexture", window_texture_);
            for (const Vec3& window_location : c_window_locations) {
                graphics::draw(transparent_mesh_, {.position = window_location}, blending_material_, camera_);
            }
        }

        camera_.render_to_screen();

        // auxiliary UI
        log_viewer_.on_draw();
        perf_panel_.on_draw();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Material opaque_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Blending.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Blending.frag"),
    }};
    Material blending_material_ = opaque_material_;
    Mesh cube_mesh_ = BoxGeometry{};
    Mesh plane_mesh_ = generate_plane();
    Mesh transparent_mesh_ = generate_transparent();
    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
    Texture2D marble_texture_ = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/marble.jpg"),
        ColorSpace::sRGB
    );
    Texture2D metal_texture_ = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );
    Texture2D window_texture_ = load_texture2D_from_image(
        loader_.open("oscar_demos/learnopengl/textures/window.png"),
        ColorSpace::sRGB
    );
    LogViewerPanel log_viewer_{"log"};
    PerfPanel perf_panel_{"perf"};
};


CStringView osc::LOGLBlendingTab::id() { return Impl::static_label(); }

osc::LOGLBlendingTab::LOGLBlendingTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLBlendingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLBlendingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLBlendingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLBlendingTab::impl_on_draw() { private_data().on_draw(); }
