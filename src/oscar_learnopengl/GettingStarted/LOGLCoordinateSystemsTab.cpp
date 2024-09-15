#include "LOGLCoordinateSystemsTab.h"

#include <oscar/oscar.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    // worldspace positions of each cube (step 2)
    constexpr auto c_cube_positions = std::to_array<Vec3>({
        { 0.0f,  0.0f,  0.0f },
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f },
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f },
        {-1.7f,  3.0f, -7.5f },
        { 1.3f, -2.0f, -2.5f },
        { 1.5f,  2.0f, -2.5f },
        { 1.5f,  0.2f, -1.5f },
        {-1.3f,  1.0f, -1.5f },
    });

    constexpr CStringView c_tab_string_id = "LearnOpenGL/CoordinateSystems";

    MouseCapturingCamera create_camera_that_matches_learnopengl()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 3.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.2f, 0.3f, 0.3f, 1.0f});
        return rv;
    }

    Material make_box_material(IResourceLoader& loader)
    {
        Material rv{Shader{
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.vert"),
            loader.slurp("oscar_learnopengl/shaders/GettingStarted/CoordinateSystems.frag"),
        }};

        rv.set(
            "uTexture1",
            load_texture2D_from_image(
                loader.open("oscar_learnopengl/textures/container.jpg"),
                ColorSpace::sRGB,
                ImageLoadingFlag::FlipVertically
            )
        );

        rv.set(
            "uTexture2",
            load_texture2D_from_image(
                loader.open("oscar_learnopengl/textures/awesomeface.png"),
                ColorSpace::sRGB,
                ImageLoadingFlag::FlipVertically
            )
        );

        return rv;
    }
}

class osc::LOGLCoordinateSystemsTab::Impl final : public StandardTabImpl {
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

    bool impl_on_event(const Event& e) final
    {
        return camera_.on_event(e);
    }

    void impl_on_tick() final
    {
        const double dt = App::get().frame_delta_since_startup().count();
        step1_transform_.rotation = angle_axis(50_deg * dt, UnitVec3{0.5f, 1.0f, 0.0f});
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

        // draw 3D scene
        if (show_step1_) {
            graphics::draw(mesh_, step1_transform_, material_, camera_);
        }
        else {
            const UnitVec3 axis{1.0f, 0.3f, 0.5f};

            for (size_t i = 0; i < c_cube_positions.size(); ++i) {
                graphics::draw(
                    mesh_,
                    Transform{
                        .rotation = angle_axis(i * 20_deg, axis),
                        .position = c_cube_positions[i],
                    },
                    material_,
                    camera_
                );
            }
        }

        camera_.render_to_screen();
    }

    void draw_2d_ui()
    {
        ui::begin_panel("Tutorial Step");
        ui::draw_checkbox("step1", &show_step1_);
        if (camera_.is_capturing_mouse()) {
            ui::draw_text("mouse captured (esc to uncapture)");
        }

        const Vec3 camera_position = camera_.position();
        ui::draw_text("camera pos = (%f, %f, %f)", camera_position.x, camera_position.y, camera_position.z);
        const EulerAngles camera_eulers = camera_.eulers();
        ui::draw_text("camera eulers = (%f, %f, %f)", camera_eulers.x.count(), camera_eulers.y.count(), camera_eulers.z.count());
        ui::end_panel();

        perf_panel_.on_draw();
    }

    ResourceLoader loader_ = App::resource_loader();
    Material material_ = make_box_material(loader_);
    Mesh mesh_ = BoxGeometry{};
    MouseCapturingCamera camera_ = create_camera_that_matches_learnopengl();
    bool show_step1_ = false;
    Transform step1_transform_;
    PerfPanel perf_panel_{"perf"};
};


CStringView osc::LOGLCoordinateSystemsTab::id()
{
    return c_tab_string_id;
}

osc::LOGLCoordinateSystemsTab::LOGLCoordinateSystemsTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLCoordinateSystemsTab::LOGLCoordinateSystemsTab(LOGLCoordinateSystemsTab&&) noexcept = default;
osc::LOGLCoordinateSystemsTab& osc::LOGLCoordinateSystemsTab::operator=(LOGLCoordinateSystemsTab&&) noexcept = default;
osc::LOGLCoordinateSystemsTab::~LOGLCoordinateSystemsTab() noexcept = default;

UID osc::LOGLCoordinateSystemsTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLCoordinateSystemsTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLCoordinateSystemsTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLCoordinateSystemsTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLCoordinateSystemsTab::impl_on_event(const Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLCoordinateSystemsTab::impl_on_tick()
{
    impl_->on_tick();
}

void osc::LOGLCoordinateSystemsTab::impl_on_draw()
{
    impl_->on_draw();
}
