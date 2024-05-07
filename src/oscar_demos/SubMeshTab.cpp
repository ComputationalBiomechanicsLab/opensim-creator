#include "SubMeshTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

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
    constexpr CStringView c_TabStringID = "Demos/SubMeshes";

    template<rgs::range T, rgs::range U>
    requires std::same_as<typename T::value_type, typename U::value_type>
    void Append(T& out, U els)
    {
        out.insert(out.end(), els.begin(), els.end());
    }

    Mesh GenerateMeshWithSubMeshes()
    {
        auto const meshes = std::to_array<Mesh>({
            BoxGeometry{2.0f, 2.0f, 2.0f},
            SphereGeometry{1.0f, 16, 16},
            CircleGeometry{1.0f, 32},
        });

        std::vector<Vec3> allVerts;
        std::vector<Vec3> allNormals;
        std::vector<uint32_t> allIndices;
        std::vector<SubMeshDescriptor> allDescriptors;

        for (auto const& mesh : meshes) {
            size_t firstVert = allVerts.size();
            Append(allVerts, mesh.vertices());
            Append(allNormals, mesh.normals());

            size_t firstIndex = allIndices.size();
            for (auto index : mesh.indices()) {
                allIndices.push_back(static_cast<uint32_t>(firstVert + index));
            }
            size_t nIndices = allIndices.size() - firstIndex;

            allDescriptors.emplace_back(firstIndex, nIndices, mesh.topology());
        }

        Mesh rv;
        rv.set_vertices(allVerts);
        rv.set_normals(allNormals);
        rv.set_indices(allIndices);
        rv.set_submesh_descriptors(allDescriptors);
        return rv;
    }
}

class osc::SubMeshTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_Camera.set_background_color(Color::white());
        m_Camera.set_near_clipping_plane(0.1f);
        m_Camera.set_far_clipping_plane(5.0f);
        m_Camera.set_position({0.0f, 0.0f, -2.5f});
        m_Camera.set_direction({0.0f, 0.0f, 1.0f});

        m_Material.set_color(Color::red());
        m_Material.set_wireframe(true);
    }

private:
    void impl_on_draw() final
    {
        for (size_t subMeshIndex = 0; subMeshIndex < m_MeshWithSubmeshes.num_submesh_descriptors(); ++subMeshIndex) {
            graphics::draw(
                m_MeshWithSubmeshes,
                identity<Transform>(),
                m_Material,
                m_Camera,
                std::nullopt,
                subMeshIndex
            );
        }
        m_Camera.set_pixel_rect(ui::get_main_viewport_workspace_screen_rect());
        m_Camera.render_to_screen();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Camera m_Camera;
    MeshBasicMaterial m_Material;
    Mesh m_MeshWithSubmeshes = GenerateMeshWithSubMeshes();
};


// public API

CStringView osc::SubMeshTab::id()
{
    return c_TabStringID;
}

osc::SubMeshTab::SubMeshTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::SubMeshTab::SubMeshTab(SubMeshTab&&) noexcept = default;
osc::SubMeshTab& osc::SubMeshTab::operator=(SubMeshTab&&) noexcept = default;
osc::SubMeshTab::~SubMeshTab() noexcept = default;

UID osc::SubMeshTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::SubMeshTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::SubMeshTab::impl_on_draw()
{
    m_Impl->on_draw();
}
