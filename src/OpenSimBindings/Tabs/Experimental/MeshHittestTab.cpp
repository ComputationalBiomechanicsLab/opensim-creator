#include "MeshHittestTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/CollisionTests.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Maths/Triangle.hpp"
#include "src/OpenSimBindings/Graphics/SimTKMeshLoader.hpp"
#include "src/Panels/PerfPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <chrono>
#include <string>
#include <utility>

class osc::MeshHittestTab::Impl final {
public:

    Impl()
    {
        m_Camera.setBackgroundColor({1.0f, 1.0f, 1.0f, 1.0f});
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
        UpdatePolarCameraFromImGuiMouseInputs(App::get().dims(), m_PolarCamera);

        // handle hittest
        auto raycastStart = std::chrono::high_resolution_clock::now();
        {
            Rect r = osc::GetMainViewportWorkspaceScreenRect();
            glm::vec2 d = osc::Dimensions(r);
            m_Ray = m_PolarCamera.unprojectTopLeftPosToWorldRay(glm::vec2{ ImGui::GetIO().MousePos } - r.p1, d);

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
                nonstd::span<glm::vec3 const> tris = m_Mesh.getVerts();
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
            glm::vec2 const viewportRectDims = osc::Dimensions(viewportRect);
            m_Camera.setPixelRect(viewportRect);

            // update real scene camera from constrained polar camera
            m_Camera.setPosition(m_PolarCamera.getPos());
            m_Camera.setNearClippingPlane(m_PolarCamera.znear);
            m_Camera.setFarClippingPlane(m_PolarCamera.zfar);
            m_Camera.setViewMatrixOverride(m_PolarCamera.getViewMtx());
            m_Camera.setProjectionMatrixOverride(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));
        }

        // draw mesh
        m_Material.setVec4("uColor", m_IsMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
        m_Material.setDepthTested(true);
        Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);

        // draw hit triangle while mousing over
        if (m_IsMousedOver)
        {
            Mesh m;
            m.setVerts(m_Tris);
            uint16_t indices[] = {0, 1, 2};
            m.setIndices(indices);

            m_Material.setVec4("uColor", {0.0f, 0.0f, 0.0f, 1.0f});
            m_Material.setDepthTested(false);
            Graphics::DrawMesh(m, Transform{}, m_Material, m_Camera);
        }

        if (m_UseBVH)
        {
            // draw BVH AABBs
            m_Material.setVec4("uColor", {0.0f, 0.0f, 0.0f, 1.0f});
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
        if (true)
        {
            ImGui::Begin("controls");
            ImGui::Checkbox("BVH", &m_UseBVH);
            ImGui::Text("%ld microseconds", static_cast<long>(m_RaycastDuration.count()));
            auto r = m_Ray;
            ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.getPosition().x, m_Camera.getPosition().y, m_Camera.getPosition().z);
            ImGui::Text("origin = (%.2f, %.2f, %.2f), dir = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z);
            if (m_IsMousedOver)
            {
                ImGui::Text("hit = (%.2f, %.2f, %.2f)", m_HitPos.x, m_HitPos.y, m_HitPos.z);
                ImGui::Text("p1 = (%.2f, %.2f, %.2f)", m_Tris[0].x, m_Tris[0].y, m_Tris[0].z);
                ImGui::Text("p2 = (%.2f, %.2f, %.2f)", m_Tris[1].x, m_Tris[1].y, m_Tris[1].z);
                ImGui::Text("p3 = (%.2f, %.2f, %.2f)", m_Tris[2].x, m_Tris[2].y, m_Tris[2].z);

            }
            ImGui::End();
        }
        m_PerfPanel.draw();
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
    Mesh m_SphereMesh = GenUntexturedUVSphere(12, 12);
    Mesh m_CubeLinesMesh = GenCubeLines();

    // other state
    bool m_UseBVH = false;
    glm::vec3 m_Tris[3]{};
    std::chrono::microseconds m_RaycastDuration{0};
    PolarPerspectiveCamera m_PolarCamera;
    bool m_IsMousedOver = false;
    glm::vec3 m_HitPos = {0.0f, 0.0f, 0.0f};
    Line m_Ray{};

    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::CStringView osc::MeshHittestTab::id() noexcept
{
    return "Hittest/Meshes";
}

osc::MeshHittestTab::MeshHittestTab(std::weak_ptr<TabHost>) :
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
