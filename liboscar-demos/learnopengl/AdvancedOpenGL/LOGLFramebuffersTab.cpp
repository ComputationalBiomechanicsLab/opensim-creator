#include "LOGLFramebuffersTab.h"

#include <liboscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    Mesh generate_plane()
    {
        Mesh mesh;
        mesh.set_vertices({
            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},

            { 5.0f, -0.5f,  5.0f},
            {-5.0f, -0.5f, -5.0f},
            { 5.0f, -0.5f, -5.0f},
        });
        mesh.set_tex_coords({
            {2.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 2.0f},

            {2.0f, 0.0f},
            {0.0f, 2.0f},
            {2.0f, 2.0f},
        });
        mesh.set_indices({0, 2, 1,    3, 5, 4});
        return mesh;
    }

    MouseCapturingCamera create_scene_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        return rv;
    }

    Camera create_screen_camera()
    {
        Camera rv;
        rv.set_view_matrix_override(identity<Mat4>());
        rv.set_projection_matrix_override(identity<Mat4>());
        return rv;
    }
}

class osc::LOGLFramebuffersTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedOpenGL/Framebuffers"; }

    explicit Impl(LOGLFramebuffersTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        scene_camera_.on_mount();
    }

    void on_unmount()
    {
        scene_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return scene_camera_.on_event(e);
    }

    void on_draw()
    {
        scene_camera_.on_draw();

        // setup render texture
        const Rect workspace_screen_space_rect = ui::get_main_window_workspace_screen_space_rect();
        const float device_pixel_ratio = App::get().main_window_device_pixel_ratio();
        const Vec2 workspace_pixel_dimensions = device_pixel_ratio * workspace_screen_space_rect.dimensions();

        render_texture_.set_pixel_dimensions(workspace_pixel_dimensions);
        render_texture_.set_device_pixel_ratio(device_pixel_ratio);
        render_texture_.set_anti_aliasing_level(App::get().anti_aliasing_level());

        // render scene
        {
            // cubes
            scene_render_material_.set("uTexture1", container_texture_);
            graphics::draw(cube_mesh_, {.position = {-1.0f, 0.0f, -1.0f}}, scene_render_material_, scene_camera_);
            graphics::draw(cube_mesh_, {.position = { 1.0f, 0.0f, -1.0f}}, scene_render_material_, scene_camera_);

            // floor
            scene_render_material_.set("uTexture1", metal_texture_);
            graphics::draw(plane_mesh_, identity<Transform>(), scene_render_material_, scene_camera_);
        }
        scene_camera_.render_to(render_texture_);

        // render via a effect sampler
        graphics::blit_to_main_window(render_texture_, screen_material_, workspace_screen_space_rect);

        // auxiliary UI
        log_viewer_.on_draw();
        perf_panel_.on_draw();
    }

private:
    ResourceLoader loader_ = App::resource_loader();

    Material scene_render_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.frag"),
    }};

    MouseCapturingCamera scene_camera_ = create_scene_camera();

    Texture2D container_texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    Texture2D metal_texture_ = Image::read_into_texture(
        loader_.open("oscar_demos/learnopengl/textures/metal.jpg"),
        ColorSpace::sRGB
    );

    Mesh cube_mesh_ = BoxGeometry{};
    Mesh plane_mesh_ = generate_plane();
    Mesh quad_mesh_ = PlaneGeometry{{.dimensions = Vec2{2.0f}}};

    RenderTexture render_texture_;
    Camera screen_camera_ = create_screen_camera();
    Material screen_material_{Shader{
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.vert"),
        loader_.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.frag"),
    }};

    LogViewerPanel log_viewer_{&owner()};
    PerfPanel perf_panel_{&owner()};
};


CStringView osc::LOGLFramebuffersTab::id() { return Impl::static_label(); }

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLFramebuffersTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLFramebuffersTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLFramebuffersTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLFramebuffersTab::impl_on_draw() { private_data().on_draw(); }
