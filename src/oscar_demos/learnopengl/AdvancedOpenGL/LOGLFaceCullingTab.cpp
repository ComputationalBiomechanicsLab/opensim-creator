#include "LOGLFaceCullingTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    Mesh generate_cube_like_learnopengl()
    {
        return BoxGeometry{};
    }

    Material generate_uv_testing_texture_mapped_material(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/FaceCulling.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/AdvancedOpenGL/FaceCulling.frag"),
        }};

        rv.set("uTexture", load_texture2D_from_image(
            loader.open("oscar_demos/learnopengl/textures/uv_checker.png"),
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

class osc::LOGLFaceCullingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/AdvancedOpenGL/FaceCulling"; }

    explicit Impl(LOGLFaceCullingTab& owner, Widget& parent) :
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
        draw_scene();
        draw_2d_ui();
    }

private:
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


CStringView osc::LOGLFaceCullingTab::id() { return Impl::static_label(); }
osc::LOGLFaceCullingTab::LOGLFaceCullingTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLFaceCullingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLFaceCullingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLFaceCullingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLFaceCullingTab::impl_on_draw() { private_data().on_draw(); }
