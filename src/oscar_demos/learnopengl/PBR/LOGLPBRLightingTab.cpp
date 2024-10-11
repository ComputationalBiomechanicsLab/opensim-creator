#include "LOGLPBRLightingTab.h"

#include <oscar/oscar.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
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
            rl.slurp("oscar_demos/learnopengl/shaders/PBR/lighting/PBR.vert"),
            rl.slurp("oscar_demos/learnopengl/shaders/PBR/lighting/PBR.frag"),
        }};
        rv.set<float>("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRLightingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/PBR/Lighting"; }

    explicit Impl(LOGLPBRLightingTab& owner, Widget& parent) :
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
        draw3DRender();
        draw_2D_ui();
    }

private:
    void draw3DRender()
    {
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        pbr_material_.set("uCameraWorldPos", camera_.position());
        pbr_material_.set_array("uLightPositions", c_light_positions);
        pbr_material_.set_array("uLightColors", c_light_radiances);

        draw_spheres();
        draw_lights();

        camera_.render_to_screen();
    }

    void draw_spheres()
    {
        pbr_material_.set("uAlbedoColor", Vec3{0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_num_rows; ++row) {
            pbr_material_.set("uMetallicity", static_cast<float>(row) / static_cast<float>(c_num_rows));

            for (int col = 0; col < c_num_cols; ++col) {
                const float normalized_col = static_cast<float>(col) / static_cast<float>(c_num_cols);
                pbr_material_.set("uRoughness", clamp(normalized_col, 0.005f, 1.0f));

                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;
                graphics::draw(sphere_mesh_, {.position = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        pbr_material_.set("uAlbedoColor", Vec3{1.0f, 1.0f, 1.0f});

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
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 64, .num_height_segments = 64}};
    Material pbr_material_ = CreateMaterial(loader_);
    PerfPanel perf_panel_{"Perf"};
};


CStringView osc::LOGLPBRLightingTab::id() { return Impl::static_label(); }

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLPBRLightingTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPBRLightingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPBRLightingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPBRLightingTab::impl_on_draw() { private_data().on_draw(); }
