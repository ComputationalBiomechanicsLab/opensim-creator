#include "MeshGenTestTab.h"

#include <oscar/oscar.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/MeshGen";

    std::vector<Vec2> LathePoints()
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

    std::map<std::string, Mesh> GenerateMeshLookup()
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
            {"lathe", LatheGeometry{LathePoints(), 3}},
            {"ring", RingGeometry{0.5f, 1.0f, 32, 3, Degrees{0}, Degrees{180}}},
        };
    }
}

class osc::MeshGenTestTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_Camera.radius = 5.0f;
    }

private:
    void impl_on_draw() final
    {
        ui::DockSpaceOverViewport(ui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_Viewer.is_hovered()) {
            ui::UpdatePolarCameraFromMouseInputs(m_Camera, App::get().dimensions());
        }

        if (ui::Begin("viewer")) {
            ui::Checkbox("is_wireframe", &m_DrawWireframe);
            for (auto const& [name, _] : m_AllMeshes) {
                if (ui::Button(name)) {
                    m_CurrentMesh = name;
                }
                ui::SameLine();
            }
            ui::NewLine();

            Vec2 contentRegion = ui::GetContentRegionAvail();
            m_RenderParams.dimensions = elementwise_max(contentRegion, {0.0f, 0.0f});
            m_RenderParams.antialiasing_level = App::get().anti_aliasing_level();

            {
                m_RenderParams.light_direction = recommended_light_direction(m_Camera);
                m_RenderParams.projection_matrix = m_Camera.projection_matrix(aspect_ratio_of(m_RenderParams.dimensions));
                m_RenderParams.view_matrix = m_Camera.view_matrix();
                m_RenderParams.view_pos = m_Camera.position();
                m_RenderParams.near_clipping_plane = m_Camera.znear;
                m_RenderParams.far_clipping_plane = m_Camera.zfar;
                m_RenderParams.draw_floor = false;
                m_RenderParams.draw_mesh_normals = true;

                m_Viewer.on_draw({{SceneDecoration{
                    .mesh = m_AllMeshes[m_CurrentMesh],
                    .color = Color::white(),
                    .flags = m_DrawWireframe ? SceneDecorationFlags::WireframeOverlay : SceneDecorationFlags::None,
                }}}, m_RenderParams);
            }
        }
        ui::End();
    }

    std::map<std::string, Mesh> m_AllMeshes = GenerateMeshLookup();
    std::string m_CurrentMesh = m_AllMeshes.begin()->first;
    bool m_DrawWireframe = false;
    SceneViewer m_Viewer;
    SceneRendererParams m_RenderParams;
    PolarPerspectiveCamera m_Camera;
};


// public API

CStringView osc::MeshGenTestTab::id()
{
    return c_TabStringID;
}

osc::MeshGenTestTab::MeshGenTestTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::MeshGenTestTab::MeshGenTestTab(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab& osc::MeshGenTestTab::operator=(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab::~MeshGenTestTab() noexcept = default;

UID osc::MeshGenTestTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::MeshGenTestTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::MeshGenTestTab::impl_on_draw()
{
    m_Impl->on_draw();
}
