#include "MeshHittestTab.h"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/Utils/UID.h>

#include <array>
#include <chrono>

using namespace osc;

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

        Rect r = GetMainViewportWorkspaceScreenRect();
        Vec2 d = dimensions(r);
        m_Ray = m_PolarCamera.unprojectTopLeftPosToWorldRay(Vec2{ImGui::GetMousePos()} - r.p1, d);

        m_IsMousedOver = false;
        if (m_UseBVH)
        {
            m_MeshBVH.forEachRayAABBCollision(m_Ray, [this](BVHCollision const& aabbColl)
            {
                Triangle const triangle = m_Mesh.getTriangleAt(aabbColl.id);
                if (auto triangleColl = find_collision(m_Ray, triangle))
                {
                    m_IsMousedOver = true;
                    m_Tris = triangle;
                }
            });
        }
        else
        {
            m_Mesh.forEachIndexedTriangle([this](Triangle triangle)
            {
                if (auto const hit = find_collision(m_Ray, triangle))
                {
                    m_HitPos = hit->position;
                    m_IsMousedOver = true;
                    m_Tris = triangle;
                }
            });
        }

        auto raycastEnd = std::chrono::high_resolution_clock::now();
        auto raycastDt = raycastEnd - raycastStart;
        m_RaycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
    }

    void onDraw()
    {
        // setup scene
        {
            Rect const viewportRect = GetMainViewportWorkspaceScreenRect();
            Vec2 const viewportRectDims = dimensions(viewportRect);
            m_Camera.setPixelRect(viewportRect);

            // update real scene camera from constrained polar camera
            m_Camera.setPosition(m_PolarCamera.getPos());
            m_Camera.setNearClippingPlane(m_PolarCamera.znear);
            m_Camera.setFarClippingPlane(m_PolarCamera.zfar);
            m_Camera.setViewMatrixOverride(m_PolarCamera.view_matrix());
            m_Camera.setProjectionMatrixOverride(m_PolarCamera.projection_matrix(AspectRatio(viewportRectDims)));
        }

        // draw mesh
        m_Material.setColor("uColor", m_IsMousedOver ? Color::green() : Color::red());
        m_Material.setDepthTested(true);
        Graphics::DrawMesh(m_Mesh, Identity<Transform>(), m_Material, m_Camera);

        // draw hit triangle while mousing over
        if (m_IsMousedOver)
        {
            Mesh m;
            m.setVerts(m_Tris);
            m.setIndices({0, 1, 2});

            m_Material.setColor("uColor", Color::black());
            m_Material.setDepthTested(false);
            Graphics::DrawMesh(m, Identity<Transform>(), m_Material, m_Camera);
        }

        if (m_UseBVH)
        {
            // draw BVH AABBs
            m_Material.setColor("uColor", Color::black());
            m_Material.setDepthTested(true);
            DrawBVH(
                *App::singleton<SceneCache>(),
                m_MeshBVH,
                [this](SceneDecoration&& dec)
                {
                    Graphics::DrawMesh(m_CubeLinesMesh, dec.transform, m_Material, m_Camera);
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
    Mesh m_Mesh = LoadMeshViaSimTK(App::resourceFilepath("geometry/hat_ribs.vtp"));
    Mesh m_SphereMesh = GenerateUVSphereMesh(12, 12);
    Mesh m_CubeLinesMesh = GenerateCubeLinesMesh();

    // other state
    BVH m_MeshBVH = CreateTriangleBVHFromMesh(m_Mesh);
    bool m_UseBVH = false;
    Triangle m_Tris;
    std::chrono::microseconds m_RaycastDuration{0};
    PolarPerspectiveCamera m_PolarCamera;
    bool m_IsMousedOver = false;
    Vec3 m_HitPos = {0.0f, 0.0f, 0.0f};
    Line m_Ray{};

    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

CStringView osc::MeshHittestTab::id()
{
    return "OpenSim/Experimental/MeshHittest";
}

osc::MeshHittestTab::MeshHittestTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MeshHittestTab::MeshHittestTab(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab& osc::MeshHittestTab::operator=(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab::~MeshHittestTab() noexcept = default;

UID osc::MeshHittestTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::MeshHittestTab::implGetName() const
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
