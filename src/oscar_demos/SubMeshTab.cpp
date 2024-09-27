#include "SubMeshTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    template<rgs::range T, rgs::range U>
    requires std::same_as<typename T::value_type, typename U::value_type>
    void append(T& out, U els)
    {
        out.insert(out.end(), els.begin(), els.end());
    }

    Mesh generate_mesh_with_submeshes()
    {
        const auto meshes = std::to_array<Mesh>({
            BoxGeometry{{.width = 2.0f, .height = 2.0f, .depth = 2.0f}},
            SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}},
            CircleGeometry{{.radius = 1.0f, .num_segments = 32}},
        });

        std::vector<Vec3> all_verts;
        std::vector<Vec3> all_normals;
        std::vector<uint32_t> all_indices;
        std::vector<SubMeshDescriptor> all_descriptors;

        for (const auto& mesh : meshes) {
            const size_t first_vert = all_verts.size();
            append(all_verts, mesh.vertices());
            append(all_normals, mesh.normals());

            const size_t first_index = all_indices.size();
            for (auto index : mesh.indices()) {
                all_indices.push_back(static_cast<uint32_t>(first_vert + index));
            }
            const size_t num_indices = all_indices.size() - first_index;

            all_descriptors.emplace_back(first_index, num_indices, mesh.topology());
        }

        Mesh rv;
        rv.set_vertices(all_verts);
        rv.set_normals(all_normals);
        rv.set_indices(all_indices);
        rv.set_submesh_descriptors(all_descriptors);
        return rv;
    }
}

class osc::SubMeshTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Demos/SubMeshes"; }

    Impl() : TabPrivate{static_label()}
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
        for (size_t submesh_index = 0; submesh_index < mesh_with_submeshes_.num_submesh_descriptors(); ++submesh_index) {
            graphics::draw(
                mesh_with_submeshes_,
                identity<Transform>(),
                material_,
                camera_,
                std::nullopt,
                submesh_index
            );
        }
        camera_.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());
        camera_.render_to_screen();
    }

private:
    ResourceLoader loader_ = App::resource_loader();
    Camera camera_;
    MeshBasicMaterial material_;
    Mesh mesh_with_submeshes_ = generate_mesh_with_submeshes();
};


CStringView osc::SubMeshTab::id() { return Impl::static_label(); }

osc::SubMeshTab::SubMeshTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}
void osc::SubMeshTab::impl_on_draw() { private_data().on_draw(); }
