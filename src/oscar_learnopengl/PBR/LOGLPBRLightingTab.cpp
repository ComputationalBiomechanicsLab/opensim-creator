#include "LOGLPBRLightingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "LearnOpenGL/PBR/Lighting";

    constexpr auto c_light_positions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_light_positions.size()> c_light_radiances = std::to_array<Vec3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_num_rows = 7;
    constexpr int c_num_cols = 7;
    constexpr float c_cell_spacing = 2.5f;

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.frag"),
        }};
        rv.set<float>("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRLightingTab::Impl final : public StandardTabImpl {
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
        draw3DRender();
        draw_2D_ui();
    }

    void draw3DRender()
    {
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        pbr_material_.set_vec3("uCameraWorldPos", camera_.position());
        pbr_material_.set_vec3_array("uLightPositions", c_light_positions);
        pbr_material_.set_vec3_array("uLightColors", c_light_radiances);

        draw_spheres();
        draw_lights();

        camera_.render_to_screen();
    }

    void draw_spheres()
    {
        pbr_material_.set_vec3("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_num_rows; ++row) {
            pbr_material_.set<float>("uMetallicity", static_cast<float>(row) / static_cast<float>(c_num_rows));

            for (int col = 0; col < c_num_cols; ++col) {
                const float normalized_col = static_cast<float>(col) / static_cast<float>(c_num_cols);
                pbr_material_.set<float>("uRoughness", clamp(normalized_col, 0.005f, 1.0f));

                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;
                graphics::draw(sphere_mesh_, {.position = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        pbr_material_.set_vec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (const Vec3& light_position : c_light_positions) {
            graphics::draw(sphere_mesh_, {.scale = Vec3{0.5f}, .position = light_position}, pbr_material_, camera_);
        }
    }

    void draw_2D_ui()
    {
        perf_panel_.on_draw();
    }

    ResourceLoader loader_ = App::resource_loader();
    MouseCapturingCamera camera_ = CreateCamera();
    Mesh sphere_mesh_ = SphereGeometry{1.0f, 64, 64};
    Material pbr_material_ = CreateMaterial(loader_);
    PerfPanel perf_panel_{"Perf"};
};


CStringView osc::LOGLPBRLightingTab::id()
{
    return c_tab_string_id;
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::LOGLPBRLightingTab::LOGLPBRLightingTab(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab& osc::LOGLPBRLightingTab::operator=(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab::~LOGLPBRLightingTab() noexcept = default;

UID osc::LOGLPBRLightingTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::LOGLPBRLightingTab::impl_get_name() const
{
    return impl_->name();
}

void osc::LOGLPBRLightingTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::LOGLPBRLightingTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::LOGLPBRLightingTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::LOGLPBRLightingTab::impl_on_draw()
{
    impl_->on_draw();
}
