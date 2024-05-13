#include "LOGLFramebuffersTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/Framebuffers";

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
        rv.set_indices({0, 2, 1,    3, 5, 4});
        return rv;
    }

    MouseCapturingCamera create_scene_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
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

class osc::LOGLFramebuffersTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        scene_camera_.on_mount();
    }

    void impl_on_unmount() final
    {
        scene_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(const SDL_Event& e) final
    {
        return scene_camera_.on_event(e);
    }

    void impl_on_draw() final
    {
        scene_camera_.on_draw();

        // setup render texture
        const Rect viewport_rect = ui::get_main_viewport_workspace_screen_rect();
        const Vec2 viewport_dimensions = dimensions_of(viewport_rect);
        render_texture_.set_dimensions(viewport_dimensions);
        render_texture_.set_anti_aliasing_level(App::get().anti_aliasing_level());

        // render scene
        {
            // cubes
            scene_render_material_.set_texture("uTexture1", container_texture_);
            graphics::draw(cube_mesh_, {.position = {-1.0f, 0.0f, -1.0f}}, scene_render_material_, scene_camera_);
            graphics::draw(cube_mesh_, {.position = { 1.0f, 0.0f, -1.0f}}, scene_render_material_, scene_camera_);

            // floor
            scene_render_material_.set_texture("uTexture1", metal_texture_);
            graphics::draw(plane_mesh_, identity<Transform>(), scene_render_material_, scene_camera_);
        }
        scene_camera_.render_to(render_texture_);

        // render via a effect sampler
        graphics::blit_to_screen(render_texture_, viewport_rect, screen_material_);

        // auxiliary UI
        log_viewer_.on_draw();
        perf_panel_.on_draw();
    }

    ResourceLoader loader_ = App::resource_loader();

    Material scene_render_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Blitter.frag"),
    }};

    MouseCapturingCamera scene_camera_ = create_scene_camera();

    Texture2D container_texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    Texture2D metal_texture_ = load_texture2D_from_image(
        loader_.open("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );

    Mesh cube_mesh_ = BoxGeometry{};
    Mesh plane_mesh_ = generate_plane();
    Mesh quad_mesh_ = PlaneGeometry{2.0f, 2.0f, 1, 1};

    RenderTexture render_texture_;
    Camera screen_camera_ = create_screen_camera();
    Material screen_material_{Shader{
        loader_.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.vert"),
        loader_.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Framebuffers/Filter.frag"),
    }};

    LogViewerPanel log_viewer_{"log"};
    PerfPanel perf_panel_{"perf"};
};


CStringView osc::LOGLFramebuffersTab::id()
{
    return c_tab_string_id;
}

osc::LOGLFramebuffersTab::LOGLFramebuffersTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLFramebuffersTab::LOGLFramebuffersTab(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab& osc::LOGLFramebuffersTab::operator=(LOGLFramebuffersTab&&) noexcept = default;
osc::LOGLFramebuffersTab::~LOGLFramebuffersTab() noexcept = default;

UID osc::LOGLFramebuffersTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLFramebuffersTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLFramebuffersTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLFramebuffersTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLFramebuffersTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLFramebuffersTab::impl_on_draw()
{
    impl_->on_draw();
}
