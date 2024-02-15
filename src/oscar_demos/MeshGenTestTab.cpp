#include "MeshGenTestTab.h"

#include <imgui.h>
#include <oscar/oscar.h>

#include <map>
#include <memory>
#include <string>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/MeshGen";

    std::map<std::string, Mesh> GenerateMeshLookup()
    {
        SceneCache cache;

        return {
            {"sphere", cache.getSphereMesh()},
            {"cylinder", cache.getCylinderMesh()},
            {"brick", cache.getBrickMesh()},
            {"cone", cache.getConeMesh()},
            {"floor", cache.getFloorMesh()},
            {"100x100 grid", cache.get100x100GridMesh()},
            {"cube (wire)", cache.getCubeWireMesh()},
            {"yline", cache.getYLineMesh()},
            {"quad", cache.getTexturedQuadMesh()},
            {"torus", cache.getTorusMesh(0.9f, 0.1f)},
            {"torusknot", GenerateTorusKnotMesh()},
            {"box", GenerateBoxMesh(2.0f, 2.0f, 2.0f, 1, 1, 1)},
            {"icosahedron", GenerateIcosahedronMesh()},
            {"dodecahedron", GenerateDodecahedronMesh()},
            {"octahedron", GenerateOctahedronMesh()},
            {"tetrahedron", GenerateTetrahedronMesh()},
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
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_Viewer.isHovered()) {
            UpdatePolarCameraFromImGuiMouseInputs(m_Camera, App::get().dims());
        }

        if (ImGui::Begin("viewer")) {
            ImGui::Checkbox("wireframe", &m_DrawWireframe);
            for (auto const& [name, _] : m_AllMeshes) {
                if (ImGui::Button(name.c_str())) {
                    m_CurrentMesh = name;
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();

            Vec2 contentRegion = ImGui::GetContentRegionAvail();
            m_RenderParams.dimensions = elementwise_max(contentRegion, {0.0f, 0.0f});
            m_RenderParams.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();

            {
                m_RenderParams.lightDirection = RecommendedLightDirection(m_Camera);
                m_RenderParams.projectionMatrix = m_Camera.getProjMtx(AspectRatio(m_RenderParams.dimensions));
                m_RenderParams.viewMatrix = m_Camera.getViewMtx();
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
        ImGui::End();
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
