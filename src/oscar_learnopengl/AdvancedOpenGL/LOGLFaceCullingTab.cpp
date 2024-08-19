#include "LOGLFaceCullingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/FaceCulling";

    Mesh generate_cube_like_learnopengl()
    {
        return BoxGeometry{};
    }

    Material generate_uv_testing_texture_mapped_material(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.vert"),
            loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.frag"),
        }};

        rv.set("uTexture", load_texture2D_from_image(
            loader.open("oscar_learnopengl/textures/uv_checker.jpg"),
            ColorSpace::sRGB
        ));

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

class osc::LOGLFaceCullingTab::Impl final : public StandardTabImpl {
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
        draw_scene();
        draw_2d_ui();
    }

    void draw_scene()
    {
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        graphics::draw(cube_, identity<Transform>(), material_, camera_);
        camera_.render_to_screen();
    }

    void draw_2d_ui()
    {
        ui::begin_panel("controls");
        if (ui::draw_button("off")) {
            material_.set_cull_mode(CullMode::Off);
        }
        if (ui::draw_button("back")) {
            material_.set_cull_mode(CullMode::Back);
        }
        if (ui::draw_button("front")) {
            material_.set_cull_mode(CullMode::Front);
        }
        ui::end_panel();
    }

    ResourceLoader loader_ = App::resource_loader();
    Material material_ = generate_uv_testing_texture_mapped_material(loader_);
    Mesh cube_ = generate_cube_like_learnopengl();
    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
};


CStringView osc::LOGLFaceCullingTab::id()
{
    return c_tab_string_id;
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLFaceCullingTab::LOGLFaceCullingTab(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab& osc::LOGLFaceCullingTab::operator=(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab::~LOGLFaceCullingTab() noexcept = default;

UID osc::LOGLFaceCullingTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLFaceCullingTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLFaceCullingTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLFaceCullingTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLFaceCullingTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLFaceCullingTab::impl_on_draw()
{
    impl_->on_draw();
}
