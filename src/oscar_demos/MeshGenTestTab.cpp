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
            {"sphere", cache.getSphereMesh()},
            {"cylinder", cache.getCylinderMesh()},
            {"brick", cache.getBrickMesh()},
            {"cone", cache.getConeMesh()},
            {"floor", cache.getFloorMesh()},
            {"circle", cache.getCircleMesh()},
            {"100x100 grid", cache.get100x100GridMesh()},
            {"cube (wire)", cache.getCubeWireMesh()},
            {"cube2 (wire)", GenerateWireframeGeometry(GenerateBoxMesh(2.0f, 2.0f, 2.0f, 1, 1, 1))},
            {"yline", cache.getYLineMesh()},
            {"quad", cache.getTexturedQuadMesh()},
            {"torus", cache.getTorusMesh(0.9f, 0.1f)},
            {"torusknot", GenerateTorusKnotMesh()},
            {"box", GenerateBoxMesh(2.0f, 2.0f, 2.0f, 1, 1, 1)},
            {"icosahedron", GenerateIcosahedronMesh()},
            {"dodecahedron", GenerateDodecahedronMesh()},
            {"octahedron", GenerateOctahedronMesh()},
            {"tetrahedron", GenerateTetrahedronMesh()},
            {"lathe", GenerateLatheMesh(LathePoints(), 3)},
            {"circle2", GenerateCircleMesh2(1.0f, 16)},
            {"ring", GenerateRingMesh(0.5f, 1.0f, 32, 3, Degrees{0}, Degrees{180})},
            {"torus2", GenerateTorusMesh2(0.9f, 0.1f, 12, 12, Degrees{360})},
            {"cylinder2", GenerateCylinderMesh2(1.0f, 1.0f, 2.0f, 16, 1)},
            {"cone2", GenerateConeMesh2(1.0f, 2.0f, 16)},
            {"plane2", GeneratePlaneMesh2(2.0f, 2.0f, 1, 1)},
            {"sphere2", GenerateSphereMesh2(1.0f, 16, 16)},
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
    void implOnDraw() final
    {
        ui::DockSpaceOverViewport(ui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_Viewer.isHovered()) {
            ui::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, App::get().dims());
        }

        if (ui::Begin("viewer")) {
            ui::Checkbox("wireframe", &m_DrawWireframe);
            for (auto const& [name, _] : m_AllMeshes) {
                if (ui::Button(name)) {
                    m_CurrentMesh = name;
                }
                ui::SameLine();
            }
            ui::NewLine();

            Vec2 contentRegion = ui::GetContentRegionAvail();
            m_RenderParams.dimensions = elementwise_max(contentRegion, {0.0f, 0.0f});
            m_RenderParams.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();

            {
                m_RenderParams.lightDirection = RecommendedLightDirection(m_Camera);
                m_RenderParams.projectionMatrix = m_Camera.projection_matrix(AspectRatio(m_RenderParams.dimensions));
                m_RenderParams.viewMatrix = m_Camera.view_matrix();
                m_RenderParams.viewPos = m_Camera.getPos();
                m_RenderParams.nearClippingPlane = m_Camera.znear;
                m_RenderParams.farClippingPlane = m_Camera.zfar;
                m_RenderParams.drawFloor = false;
                m_RenderParams.drawMeshNormals = true;

                m_Viewer.onDraw({{SceneDecoration{
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

UID osc::MeshGenTestTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::MeshGenTestTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MeshGenTestTab::implOnDraw()
{
    m_Impl->onDraw();
}
