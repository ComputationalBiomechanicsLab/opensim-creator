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

namespace
{
    constexpr CStringView c_TabStringID = "Demos/SubMeshes";

    template<std::ranges::range T, std::ranges::range U>
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
            Append(allVerts, mesh.getVerts());
            Append(allNormals, mesh.getNormals());

            size_t firstIndex = allIndices.size();
            for (auto index : mesh.getIndices()) {
                allIndices.push_back(static_cast<uint32_t>(firstVert + index));
            }
            size_t nIndices = allIndices.size() - firstIndex;

            allDescriptors.emplace_back(firstIndex, nIndices, mesh.getTopology());
        }

        Mesh rv;
        rv.setVerts(allVerts);
        rv.setNormals(allNormals);
        rv.setIndices(allIndices);
        rv.setSubmeshDescriptors(allDescriptors);
        return rv;
    }
}

class osc::SubMeshTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_Camera.setBackgroundColor(Color::white());
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(5.0f);
        m_Camera.setPosition({0.0f, 0.0f, -2.5f});
        m_Camera.setDirection({0.0f, 0.0f, 1.0f});

        m_Material.setColor(Color::red());
        m_Material.setWireframeMode(true);
    }

private:
    void implOnDraw() final
    {
        for (size_t subMeshIndex = 0; subMeshIndex < m_MeshWithSubmeshes.getSubMeshCount(); ++subMeshIndex) {
            graphics::drawMesh(
                m_MeshWithSubmeshes,
                identity<Transform>(),
                m_Material,
                m_Camera,
                std::nullopt,
                subMeshIndex
            );
        }
        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
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

UID osc::SubMeshTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::SubMeshTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::SubMeshTab::implOnDraw()
{
    m_Impl->onDraw();
}
