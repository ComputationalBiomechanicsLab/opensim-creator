#include "MeshGenTestTab.h"

#include <oscar/oscar.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "Demos/MeshGen";

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
            {"box", BoxGeometry{2.0f, 2.0f, 2.0f, 1, 1, 1}},
            {"icosahedron", IcosahedronGeometry{}},
            {"dodecahedron", DodecahedronGeometry{}},
            {"octahedron", OctahedronGeometry{}},
            {"tetrahedron", TetrahedronGeometry{}},
            {"lathe", LatheGeometry{generate_lathe_points(), 3}},
            {"ring", RingGeometry{0.5f, 1.0f, 32, 3, Degrees{0}, Degrees{180}}},
        };
    }
}

class osc::MeshGenTestTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {
        camera_.radius = 5.0f;
    }

private:
    void impl_on_draw() final
    {
        ui::enable_dockspace_over_viewport(ui::get_main_viewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (viewer_.is_hovered()) {
            ui::update_polar_camera_from_mouse_inputs(camera_, App::get().dimensions());
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

            Vec2 content_region = ui::get_content_region_avail();
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
                .color = Color::white(),
                .flags = draw_wireframe_ ? SceneDecorationFlags::WireframeOverlay : SceneDecorationFlags::None,
            }}}, render_params_);
        }
        ui::end_panel();
    }

    std::map<std::string, Mesh> all_meshes_ = generate_mesh_lookup();
    std::string current_mesh_ = all_meshes_.begin()->first;
    bool draw_wireframe_ = false;
    SceneViewer viewer_;
    SceneRendererParams render_params_;
    PolarPerspectiveCamera camera_;
};


CStringView osc::MeshGenTestTab::id()
{
    return c_tab_string_id;
}

osc::MeshGenTestTab::MeshGenTestTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}

osc::MeshGenTestTab::MeshGenTestTab(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab& osc::MeshGenTestTab::operator=(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab::~MeshGenTestTab() noexcept = default;

UID osc::MeshGenTestTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::MeshGenTestTab::impl_get_name() const
{
    return impl_->name();
}

void osc::MeshGenTestTab::impl_on_draw()
{
    impl_->on_draw();
}
