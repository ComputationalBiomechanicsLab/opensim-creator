#include "MeshHittestTab.hpp"

#include <OpenSimCreator/Bindings/SimTKMeshLoader.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/CollisionTests.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Triangle.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/Utils/UID.hpp>

#include <array>
#include <chrono>
#include <string>
#include <utility>

class osc::MeshHittestTab::Impl final {
public:

    Impl()
    {
        m_Camera.setBackgroundColor(Color::white());
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " MeshHittestTab";
    }

    void onTick()
    {
        UpdatePolarCameraFromImGuiMouseInputs(m_PolarCamera, App::get().dims());

        // handle hittest
        auto raycastStart = std::chrono::high_resolution_clock::now();
        {
            Rect r = osc::GetMainViewportWorkspaceScreenRect();
            Vec2 d = osc::Dimensions(r);
            m_Ray = m_PolarCamera.unprojectTopLeftPosToWorldRay(Vec2{ImGui::GetMousePos()} - r.p1, d);

            m_IsMousedOver = false;
            if (m_UseBVH)
            {
                MeshIndicesView const indices = m_Mesh.getIndices();
                std::optional<BVHCollision> const maybeCollision = indices.isU16() ?
                    m_Mesh.getBVH().getClosestRayIndexedTriangleCollision(m_Mesh.getVerts(), indices.toU16Span(), m_Ray) :
                    m_Mesh.getBVH().getClosestRayIndexedTriangleCollision(m_Mesh.getVerts(), indices.toU32Span(), m_Ray);
                if (maybeCollision)
                {
                    uint32_t index = m_Mesh.getIndices()[maybeCollision->id];
                    m_IsMousedOver = true;
                    m_Tris[0] = m_Mesh.getVerts()[index];
                    m_Tris[1] = m_Mesh.getVerts()[index+1];
                    m_Tris[2] = m_Mesh.getVerts()[index+2];
                }
            }
            else
            {
                std::span<Vec3 const> tris = m_Mesh.getVerts();
                for (size_t i = 0; i < tris.size(); i += 3)
                {
                    std::optional<RayCollision> const res = GetRayCollisionTriangle(
                        m_Ray,
                        Triangle{tris[i], tris[i+1], tris[i+2]}
                    );
                    if (res)
                    {
                        m_HitPos = res->position;
                        m_IsMousedOver = true;

                        m_Tris[0] = tris[i];
                        m_Tris[1] = tris[i + 1];
                        m_Tris[2] = tris[i + 2];

                        break;
                    }
                }
            }
        }
        auto raycastEnd = std::chrono::high_resolution_clock::now();
        auto raycastDt = raycastEnd - raycastStart;
        m_RaycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
    }

    void onDraw()
    {
        // setup scene
        {
            Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
            Vec2 const viewportRectDims = osc::Dimensions(viewportRect);
            m_Camera.setPixelRect(viewportRect);

            // update real scene camera from constrained polar camera
            m_Camera.setPosition(m_PolarCamera.getPos());
            m_Camera.setNearClippingPlane(m_PolarCamera.znear);
            m_Camera.setFarClippingPlane(m_PolarCamera.zfar);
            m_Camera.setViewMatrixOverride(m_PolarCamera.getViewMtx());
            m_Camera.setProjectionMatrixOverride(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));
        }

        // draw mesh
        m_Material.setColor("uColor", m_IsMousedOver ? Color::green() : Color::red());
        m_Material.setDepthTested(true);
        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);

        // draw hit triangle while mousing over
        if (m_IsMousedOver)
        {
            Mesh m;
            m.setVerts(m_Tris);
            m.setIndices(std::to_array<uint16_t>({0, 1, 2}));

            m_Material.setColor("uColor", Color::black());
            m_Material.setDepthTested(false);
            Graphics::DrawMesh(m, Transform{}, m_Material, m_Camera);
        }

        if (m_UseBVH)
        {
            // draw BVH AABBs
            m_Material.setColor("uColor", Color::black());
            m_Material.setDepthTested(true);
            osc::DrawBVH(
                *App::singleton<MeshCache>(),
                m_Mesh.getBVH(),
                [this](osc::SceneDecoration&& dec)
                {
                    osc::Graphics::DrawMesh(m_CubeLinesMesh, dec.transform, m_Material, m_Camera);
                }
            );
        }

        // draw scene onto viewport
        m_Camera.renderToScreen();

        // auxiliary 2D UI
        // printout stats
        {
            ImGui::Begin("controls");
            ImGui::Checkbox("BVH", &m_UseBVH);
            ImGui::Text("%ld microseconds", static_cast<long>(m_RaycastDuration.count()));
            auto r = m_Ray;
            ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.getPosition().x, m_Camera.getPosition().y, m_Camera.getPosition().z);
            ImGui::Text("origin = (%.2f, %.2f, %.2f), direction = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.direction.x, r.direction.y, r.direction.z);
            if (m_IsMousedOver)
            {
                ImGui::Text("hit = (%.2f, %.2f, %.2f)", m_HitPos.x, m_HitPos.y, m_HitPos.z);
                ImGui::Text("p1 = (%.2f, %.2f, %.2f)", m_Tris[0].x, m_Tris[0].y, m_Tris[0].z);
                ImGui::Text("p2 = (%.2f, %.2f, %.2f)", m_Tris[1].x, m_Tris[1].y, m_Tris[1].z);
                ImGui::Text("p3 = (%.2f, %.2f, %.2f)", m_Tris[2].x, m_Tris[2].y, m_Tris[2].z);

            }
            ImGui::End();
        }
        m_PerfPanel.onDraw();
    }

private:

    UID m_TabID;

    // rendering
    Camera m_Camera;
    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/SolidColor.vert"),
            App::slurp("shaders/SolidColor.frag"),
        },
    };
    Mesh m_Mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs.vtp"));
    Mesh m_SphereMesh = GenSphere(12, 12);
    Mesh m_CubeLinesMesh = GenCubeLines();

    // other state
    bool m_UseBVH = false;
    std::array<Vec3, 3> m_Tris{};
    std::chrono::microseconds m_RaycastDuration{0};
    PolarPerspectiveCamera m_PolarCamera;
    bool m_IsMousedOver = false;
    Vec3 m_HitPos = {0.0f, 0.0f, 0.0f};
    Line m_Ray{};

    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::CStringView osc::MeshHittestTab::id() noexcept
{
    return "OpenSim/Experimental/MeshHittest";
}

osc::MeshHittestTab::MeshHittestTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MeshHittestTab::MeshHittestTab(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab& osc::MeshHittestTab::operator=(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab::~MeshHittestTab() noexcept = default;

osc::UID osc::MeshHittestTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshHittestTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MeshHittestTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MeshHittestTab::implOnDraw()
{
    m_Impl->onDraw();
}
