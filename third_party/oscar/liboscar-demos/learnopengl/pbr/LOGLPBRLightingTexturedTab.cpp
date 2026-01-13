#include "LOGLPBRLightingTexturedTab.h"

#include <liboscar/formats/Image.h>
#include <liboscar/graphics/geometries/SphereGeometry.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/platform/App.h>
#include <liboscar/platform/ResourceLoader.h>
#include <liboscar/ui/MouseCapturingCamera.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/PerfPanel.h>
#include <liboscar/ui/tabs/TabPrivate.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_light_positions = std::to_array<Vector3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vector3, c_light_positions.size()> c_light_radiances = std::to_array<Vector3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_num_rows = 7;
    constexpr int c_num_cols = 7;
    constexpr float c_cell_spacing = 2.5f;

    MouseCapturingCamera create_camera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_field_of_view(45_deg);
        rv.set_clipping_planes({0.1f, 100.0f});
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material create_material(ResourceLoader& loader)
    {
        const Texture2D albedo = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/pbr/rusted_iron/albedo.jpg"),
            ColorSpace::sRGB
        );
        const Texture2D normal = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/pbr/rusted_iron/normal.jpg"),
            ColorSpace::Linear,
            ImageLoadingFlag::TreatComponentsAsSpatialVectors
        );
        const Texture2D metallic = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/pbr/rusted_iron/metallic.jpg"),
            ColorSpace::Linear
        );
        const Texture2D roughness = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/pbr/rusted_iron/roughness.jpg"),
            ColorSpace::Linear
        );
        const Texture2D ao = Image::read_into_texture(
            loader.open("oscar_demos/learnopengl/textures/pbr/rusted_iron/ao.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/lighting_textured/PBR.vert"),
            loader.slurp("oscar_demos/learnopengl/shaders/PBR/lighting_textured/PBR.frag"),
        }};
        rv.set("uAlbedoMap", albedo);
        rv.set("uNormalMap", normal);
        rv.set("uMetallicMap", metallic);
        rv.set("uRoughnessMap", roughness);
        rv.set("uAOMap", ao);
        rv.set_array("uLightWorldPositions", c_light_positions);
        rv.set_array("uLightRadiances", c_light_radiances);
        return rv;
    }
}

class osc::LOGLPBRLightingTexturedTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/learnopengl/PBR/LightingTextured"; }

    explicit Impl(LOGLPBRLightingTexturedTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        sphere_mesh_.recalculate_tangents();  // normal mapping
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
        draw_3d_render();
        perf_panel_.on_draw();
    }

private:
    void draw_3d_render()
    {
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());

        pbr_material_.set("uCameraWorldPosition", camera_.position());

        draw_spheres();
        draw_lights();

        camera_.render_to_main_window();
    }

    void draw_spheres()
    {
        for (int row = 0; row < c_num_rows; ++row) {
            for (int col = 0; col < c_num_cols; ++col) {
                const float x = (static_cast<float>(col) - static_cast<float>(c_num_cols)/2.0f) * c_cell_spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(c_num_rows)/2.0f) * c_cell_spacing;
                graphics::draw(sphere_mesh_, {.translation = {x, y, 0.0f}}, pbr_material_, camera_);
            }
        }
    }

    void draw_lights()
    {
        for (const Vector3& light_position : c_light_positions) {
            graphics::draw(sphere_mesh_, {.scale = Vector3{0.5f}, .translation = light_position}, pbr_material_, camera_);
        }
    }

    ResourceLoader loader_ = App::resource_loader();
    MouseCapturingCamera camera_ = create_camera();
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 64, .num_height_segments = 64}};
    Material pbr_material_ = create_material(loader_);
    PerfPanel perf_panel_{&owner()};
};


CStringView osc::LOGLPBRLightingTexturedTab::id() { return Impl::static_label(); }

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::LOGLPBRLightingTexturedTab::impl_on_mount() { private_data().on_mount(); }
void osc::LOGLPBRLightingTexturedTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::LOGLPBRLightingTexturedTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::LOGLPBRLightingTexturedTab::impl_on_draw() { private_data().on_draw(); }
