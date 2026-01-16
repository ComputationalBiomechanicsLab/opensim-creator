#include "mesh_gen_test_tab.h"

#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/geometries/dodecahedron_geometry.h>
#include <liboscar/graphics/geometries/icosahedron_geometry.h>
#include <liboscar/graphics/geometries/lathe_geometry.h>
#include <liboscar/graphics/geometries/octahedron_geometry.h>
#include <liboscar/graphics/geometries/plane_geometry.h>
#include <liboscar/graphics/geometries/ring_geometry.h>
#include <liboscar/graphics/geometries/tetrahedron_geometry.h>
#include <liboscar/graphics/geometries/torus_knot_geometry.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/camera_view_axes.h>
#include <liboscar/ui/widgets/scene_viewer.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace osc;

namespace
{
    std::vector<Vector2> generate_lathe_points()
    {
        std::vector<Vector2> rv;
        rv.reserve(10);
        for (size_t i = 0; i < 10; ++i) {
            const float x = sin(0.2f * static_cast<float>(i)) * 10.0f + 5.0f;
            const float y = (static_cast<float>(i) - 5.0f) * 2.0f;
            rv.emplace_back(x, y);
        }
        return rv;
    }

    std::map<std::string, Mesh> generate_mesh_lookup()
    {
        SceneCache cache;

        return {
            {"sphere", cache.sphere_mesh()},
            {"cylinder", cache.cylinder_mesh()},
            {"brick", cache.brick_mesh()},
            {"cone", cache.cone_mesh()},
            {"floor", cache.floor_mesh()},
            {"circle", cache.circle_mesh()},
            {"100x100 grid", cache.grid_mesh()},
            {"cube (wire)", cache.cube_wireframe_mesh()},
            {"y-line", cache.yline_mesh()},
            {"quad", cache.quad_mesh()},
            {"torus", cache.torus_mesh(0.9f, 0.1f)},
            {"plane", PlaneGeometry{}.mesh()},
            {"torus_knot", TorusKnotGeometry{}.mesh()},
            {"box", BoxGeometry{{.dimensions = Vector3{2.0f}}}},
            {"icosahedron", IcosahedronGeometry{}.mesh()},
            {"dodecahedron", DodecahedronGeometry{}.mesh()},
            {"octahedron", OctahedronGeometry{}.mesh()},
            {"tetrahedron", TetrahedronGeometry{}.mesh()},
            {"lathe", LatheGeometry{{.points = generate_lathe_points(), .num_segments = 3}}.mesh()},
            {"ring", RingGeometry{{.num_phi_segments = 3, .theta_length = Degrees{180}}}.mesh()},
        };
    }
}

class osc::MeshGenTestTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/MeshGenTest"; }

    explicit Impl(MeshGenTestTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        camera_.radius = 5.0f;
    }

    void on_draw()
    {
        ui::enable_dockspace_over_main_window();

        if (viewer_.is_hovered()) {
            ui::update_polar_camera_from_mouse_inputs(camera_, App::get().main_window_dimensions());
        }

        if (ui::begin_panel("viewer")) {
            ui::draw_checkbox("is_wireframe", &draw_wireframe_);
            for (const auto& [name, _] : all_meshes_) {
                if (ui::draw_button(name)) {
                    current_mesh_ = name;
                }
                ui::same_line();
            }
            ui::start_new_line();

            const Rect viewport_ui_rect = ui::get_content_region_available_ui_rect();
            const Vector2 viewport_dimensions = viewport_ui_rect.dimensions();
            render_params_.dimensions = elementwise_max(viewport_dimensions, {0.0f, 0.0f});
            render_params_.device_pixel_ratio = App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio(),
            render_params_.anti_aliasing_level = App::get().anti_aliasing_level();
            render_params_.light_direction = recommended_light_direction(camera_);
            render_params_.projection_matrix = camera_.projection_matrix(aspect_ratio_of(render_params_.dimensions));
            render_params_.view_matrix = camera_.view_matrix();
            render_params_.viewer_position = camera_.position();
            render_params_.near_clipping_plane = camera_.znear;
            render_params_.far_clipping_plane = camera_.zfar;
            render_params_.draw_floor = false;
            render_params_.draw_mesh_normals = true;

            viewer_.on_draw({{SceneDecoration{
                .mesh = all_meshes_[current_mesh_],
                .shading = Color::white(),
                .flags = draw_wireframe_ ? SceneDecorationFlag::WireframeOverlayedDefault : SceneDecorationFlag::Default,
            }}}, render_params_);

            // Draw camera manipulator
            ui::set_cursor_ui_position(viewport_ui_rect.ypd_top_right() - Vector2{camera_axes_ui_.dimensions().x, 0.0f});
            camera_axes_ui_.draw(camera_);
        }
        ui::end_panel();
    }

private:
    std::map<std::string, Mesh> all_meshes_ = generate_mesh_lookup();
    std::string current_mesh_ = all_meshes_.begin()->first;
    bool draw_wireframe_ = false;
    SceneViewer viewer_;
    SceneRendererParams render_params_;
    PolarPerspectiveCamera camera_;
    CameraViewAxes camera_axes_ui_;
};


CStringView osc::MeshGenTestTab::id() { return Impl::static_label(); }

osc::MeshGenTestTab::MeshGenTestTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::MeshGenTestTab::impl_on_draw() { private_data().on_draw(); }
