#include "SubMeshTab.h"

#include <liboscar/graphics/Camera.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/CircleGeometry.h>
#include <liboscar/graphics/geometries/SphereGeometry.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/materials/MeshBasicMaterial.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/SubMeshDescriptor.h>
#include <liboscar/platform/App.h>
#include <liboscar/platform/ResourceLoader.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/TabPrivate.h>

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    template<rgs::range T, rgs::range U>
    requires std::same_as<typename T::value_type, typename U::value_type>
    void insert_at_back(T& out, U els)
    {
        out.insert(out.end(), els.begin(), els.end());
    }

    Mesh generate_mesh_with_sub_meshes()
    {
        const auto meshes = std::to_array<Mesh>({
            BoxGeometry{{.dimensions = Vector3{2.0f}}},
            SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}},
            CircleGeometry{{.radius = 1.0f, .num_segments = 32}},
        });

        std::vector<Vector3> all_vertices;
        std::vector<Vector3> all_normals;
        std::vector<uint32_t> all_indices;
        std::vector<SubMeshDescriptor> all_descriptors;

        for (const auto& mesh : meshes) {
            const size_t first_vert = all_vertices.size();
            insert_at_back(all_vertices, mesh.vertices());
            insert_at_back(all_normals, mesh.normals());

            const size_t first_index = all_indices.size();
            for (const auto index : mesh.indices()) {
                all_indices.push_back(static_cast<uint32_t>(first_vert + index));
            }
            const size_t num_indices = all_indices.size() - first_index;

            all_descriptors.emplace_back(first_index, num_indices, mesh.topology());
        }

        Mesh rv;
        rv.set_vertices(all_vertices);
        rv.set_normals(all_normals);
        rv.set_indices(all_indices);
        rv.set_submesh_descriptors(all_descriptors);
        return rv;
    }
}

class osc::SubMeshTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/SubMesh"; }

    explicit Impl(SubMeshTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        camera_.set_background_color(Color::white());
        camera_.set_clipping_planes({0.1f, 5.0f});
        camera_.set_position({0.0f, 0.0f, -2.5f});
        camera_.set_direction({0.0f, 0.0f, 1.0f});

        material_.set_color(Color::red());
        material_.set_wireframe(true);
    }

    void on_draw()
    {
        const size_t num_sub_mesh_descriptors = mesh_with_sub_meshes_.num_submesh_descriptors();
        for (size_t sub_mesh_index = 0; sub_mesh_index < num_sub_mesh_descriptors; ++sub_mesh_index) {
            graphics::draw(
                mesh_with_sub_meshes_,
                identity<Transform>(),
                material_,
                camera_,
                sub_mesh_index
            );
        }
        camera_.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());
        camera_.render_to_main_window();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Camera camera_;
    MeshBasicMaterial material_;
    Mesh mesh_with_sub_meshes_ = generate_mesh_with_sub_meshes();
};


CStringView osc::SubMeshTab::id() { return Impl::static_label(); }

osc::SubMeshTab::SubMeshTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::SubMeshTab::impl_on_draw() { private_data().on_draw(); }
