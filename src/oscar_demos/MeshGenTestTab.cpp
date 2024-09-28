#include "MeshGenTestTab.h"

#include <oscar/oscar.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace osc;

namespace
{
    std::vector<Vec2> generate_lathe_points()
    {
        std::vector<Vec2> rv;
        rv.reserve(10);
        for (size_t i = 0; i < 10; ++i) {
            rv.emplace_back(
                sin(static_cast<float>(i) * 0.2f) * 10.0f + 5.0f,
                (static_cast<float>(i) - 5.0f) * 2.0f
            );
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
            {"yline", cache.yline_mesh()},
            {"quad", cache.quad_mesh()},
            {"torus", cache.torus_mesh(0.9f, 0.1f)},
            {"torusknot", TorusKnotGeometry{}},
            {"box", BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}}},
            {"icosahedron", IcosahedronGeometry{}},
            {"dodecahedron", DodecahedronGeometry{}},
            {"octahedron", OctahedronGeometry{}},
            {"tetrahedron", TetrahedronGeometry{}},
            {"lathe", LatheGeometry{{.points = generate_lathe_points(), .num_segments = 3}}},
            {"ring", RingGeometry{{.num_phi_segments = 3, .theta_length = Degrees{180}}}},
        };
    }
}

class osc::MeshGenTestTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Demos/MeshGen"; }

    explicit Impl(MeshGenTestTab& owner) :
        TabPrivate{owner, static_label()}
    {
        camera_.radius = 5.0f;
    }

    void on_draw()
    {
        ui::enable_dockspace_over_main_viewport();

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

            Vec2 content_region = ui::get_content_region_available();
            render_params_.dimensions = elementwise_max(content_region, {0.0f, 0.0f});
            render_params_.antialiasing_level = App::get().anti_aliasing_level();
            render_params_.light_direction = recommended_light_direction(camera_);
            render_params_.projection_matrix = camera_.projection_matrix(aspect_ratio_of(render_params_.dimensions));
            render_params_.view_matrix = camera_.view_matrix();
            render_params_.view_pos = camera_.position();
            render_params_.near_clipping_plane = camera_.znear;
            render_params_.far_clipping_plane = camera_.zfar;
            render_params_.draw_floor = false;
            render_params_.draw_mesh_normals = true;

            viewer_.on_draw({{SceneDecoration{
                .mesh = all_meshes_[current_mesh_],
                .shading = Color::white(),
                .flags = draw_wireframe_ ? SceneDecorationFlag::WireframeOverlayedDefault : SceneDecorationFlag::Default,
            }}}, render_params_);
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
};


CStringView osc::MeshGenTestTab::id() { return Impl::static_label(); }

osc::MeshGenTestTab::MeshGenTestTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>(*this)}
{}
void osc::MeshGenTestTab::impl_on_draw() { private_data().on_draw(); }

