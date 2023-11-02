#include "MeshGenTestTab.hpp"

#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Maths/Constants.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Scene/SceneRenderer.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/UI/Widgets/SceneViewer.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <map>
#include <memory>
#include <string>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/MeshGen";

    std::map<std::string, osc::Mesh> GenerateMeshLookup()
    {
        osc::MeshCache& cache = *osc::App::singleton<osc::MeshCache>();
        return
        {
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
        };
    }
}

class osc::MeshGenTestTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        m_Camera.radius = 5.0f;
    }

private:
    void implOnDraw() final
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_Viewer.isHovered())
        {
            osc::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, App::get().dims());
        }

        if (ImGui::Begin("viewer"))
        {
            for (auto const& [name, _] : m_AllMeshes)
            {
                if (ImGui::Button(name.c_str()))
                {
                    m_CurrentMesh = name;
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();

            Vec2 contentRegion = ImGui::GetContentRegionAvail();
            m_RenderParams.dimensions = osc::Max(contentRegion, {0.0f, 0.0f});
            m_RenderParams.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();

            {
                m_RenderParams.lightDirection = osc::RecommendedLightDirection(m_Camera);
                m_RenderParams.projectionMatrix = m_Camera.getProjMtx(AspectRatio(m_RenderParams.dimensions));
                m_RenderParams.viewMatrix = m_Camera.getViewMtx();
                m_RenderParams.viewPos = m_Camera.getPos();
                m_RenderParams.nearClippingPlane = m_Camera.znear;
                m_RenderParams.farClippingPlane = m_Camera.zfar;
                m_RenderParams.drawFloor = false;
                m_RenderParams.drawMeshNormals = true;

                SceneDecoration d
                {
                    m_AllMeshes[m_CurrentMesh],
                    Transform{},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    "NO_ID",
                    SceneDecorationFlags{}
                };

                m_Viewer.onDraw(nonstd::span<SceneDecoration const>{&d, 1}, m_RenderParams);
            }
        }
        ImGui::End();
    }

    std::string m_CurrentMesh = "brick";
    std::map<std::string, osc::Mesh> m_AllMeshes = GenerateMeshLookup();
    SceneViewer m_Viewer;
    SceneRendererParams m_RenderParams;
    PolarPerspectiveCamera m_Camera;
};


// public API

osc::CStringView osc::MeshGenTestTab::id() noexcept
{
    return c_TabStringID;
}

osc::MeshGenTestTab::MeshGenTestTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MeshGenTestTab::MeshGenTestTab(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab& osc::MeshGenTestTab::operator=(MeshGenTestTab&&) noexcept = default;
osc::MeshGenTestTab::~MeshGenTestTab() noexcept = default;

osc::UID osc::MeshGenTestTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshGenTestTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MeshGenTestTab::implOnDraw()
{
    m_Impl->onDraw();
}
