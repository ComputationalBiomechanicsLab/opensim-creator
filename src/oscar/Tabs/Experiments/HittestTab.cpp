#include "HittestTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MaterialPropertyBlock.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Maths/CollisionTests.hpp"
#include "oscar/Maths/Disc.hpp"
#include "oscar/Maths/Line.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Sphere.hpp"
#include "oscar/Maths/Triangle.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <IconsFontAwesome5.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Experiments/Hittest";

    constexpr auto c_CrosshairVerts = osc::to_array<glm::vec3>(
    {
        // -X to +X
        {-0.05f, 0.0f, 0.0f},
        {+0.05f, 0.0f, 0.0f},

        // -Y to +Y
        {0.0f, -0.05f, 0.0f},
        {0.0f, +0.05f, 0.0f},
    });

    constexpr auto c_CrosshairIndices = osc::to_array<uint16_t>({ 0, 1, 2, 3 });

    constexpr auto c_TriangleVerts = osc::to_array<glm::vec3>(
    {
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    });

    constexpr auto c_TriangleIndices = osc::to_array<uint16_t>({ 0, 1, 2 });

    struct SceneSphere final {

        explicit SceneSphere(glm::vec3 pos_) :
            pos{pos_}
        {
        }

        glm::vec3 pos;
        bool isHovered = false;
    };

    std::vector<SceneSphere> GenerateSceneSpheres()
    {
        constexpr int32_t min = -30;
        constexpr int32_t max = 30;
        constexpr int32_t step = 6;

        std::vector<SceneSphere> rv;
        for (int32_t x = min; x <= max; x += step)
        {
            for (int32_t y = min; y <= max; y += step)
            {
                for (int32_t z = min; z <= max; z += step)
                {
                    rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
                }
            }
        }
        return rv;
    }

    osc::Mesh GenerateCrosshairMesh()
    {
        osc::Mesh rv;
        rv.setTopology(osc::MeshTopology::Lines);
        rv.setVerts(c_CrosshairVerts);
        rv.setIndices(c_CrosshairIndices);
        return rv;
    }

    osc::Mesh GenerateTriangleMesh()
    {
        osc::Mesh rv;
        rv.setVerts(c_TriangleVerts);
        rv.setIndices(c_TriangleIndices);
        return rv;
    }

    osc::MaterialPropertyBlock GeneratePropertyBlock(osc::Color const& color)
    {
        osc::MaterialPropertyBlock p;
        p.setColor("uColor", color);
        return p;
    }

    osc::Line GetCameraRay(osc::Camera const& camera)
    {
        return
        {
            camera.getPosition(),
            camera.getDirection(),
        };
    }
}

class osc::HittestTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        m_Camera.setBackgroundColor({1.0f, 1.0f, 1.0f, 0.0f});
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().makeMainEventLoopWaiting();
        App::upd().setShowCursor(true);
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void implOnTick() final
    {
        // hittest spheres

        Line ray = GetCameraRay(m_Camera);
        float closestEl = std::numeric_limits<float>::max();
        SceneSphere* closestSceneSphere = nullptr;

        for (SceneSphere& ss : m_SceneSpheres)
        {
            ss.isHovered = false;

            Sphere s
            {
                ss.pos,
                m_SceneSphereBoundingSphere.radius
            };

            std::optional<RayCollision> res = GetRayCollisionSphere(ray, s);
            if (res && res->distance >= 0.0f && res->distance < closestEl)
            {
                closestEl = res->distance;
                closestSceneSphere = &ss;
            }
        }

        if (closestSceneSphere)
        {
            closestSceneSphere->isHovered = true;
        }
    }

    void implOnDraw() final
    {
        // handle mouse capturing
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // render spheres
        for (SceneSphere const& sphere : m_SceneSpheres)
        {
            Transform t;
            t.position = sphere.pos;

            Graphics::DrawMesh(
                m_SphereMesh,
                t,
                m_Material,
                m_Camera,
                sphere.isHovered ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );

            // draw sphere AABBs
            if (m_IsShowingAABBs)
            {
                Transform aabbTransform;
                aabbTransform.position = sphere.pos;
                aabbTransform.scale = 0.5f * Dimensions(m_SceneSphereAABB);

                Graphics::DrawMesh(
                    m_WireframeCubeMesh,
                    aabbTransform,
                    m_Material,
                    m_Camera,
                    m_BlackColorMaterialProps
                );
            }
        }

        // hittest + draw disc
        {
            Line const ray = GetCameraRay(m_Camera);

            Disc sceneDisc{};
            sceneDisc.origin = {0.0f, 0.0f, 0.0f};
            sceneDisc.normal = {0.0f, 1.0f, 0.0f};
            sceneDisc.radius = {10.0f};

            std::optional<RayCollision> maybeCollision = GetRayCollisionDisc(ray, sceneDisc);

            Disc meshDisc{};
            meshDisc.origin = {0.0f, 0.0f, 0.0f};
            meshDisc.normal = {0.0f, 0.0f, 1.0f};
            meshDisc.radius = 1.0f;

            Graphics::DrawMesh(
                m_CircleMesh,
                DiscToDiscMat4(meshDisc, sceneDisc),
                m_Material,
                m_Camera,
                maybeCollision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );
        }

        // hittest + draw triangle
        {
            Line ray = GetCameraRay(m_Camera);
            std::optional<RayCollision> maybeCollision = GetRayCollisionTriangle(
                ray,
                Triangle{c_TriangleVerts.at(0), c_TriangleVerts.at(1), c_TriangleVerts.at(2)}
            );

            Graphics::DrawMesh(
                m_TriangleMesh,
                Transform{},
                m_Material,
                m_Camera,
                maybeCollision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );
        }

        // draw crosshair overlay
        Graphics::DrawMesh(
            m_CrosshairMesh,
            m_Camera.getInverseViewProjectionMatrix(App::get().aspectRatio()),
            m_Material,
            m_Camera,
            m_BlackColorMaterialProps
        );

        // draw scene to screen
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    Camera m_Camera;
    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/SolidColor.vert"),
            App::slurp("shaders/SolidColor.frag"),
        },
    };
    Mesh m_SphereMesh = GenSphere(12, 12);
    Mesh m_WireframeCubeMesh = GenCubeLines();
    Mesh m_CircleMesh = GenCircle(36);
    Mesh m_CrosshairMesh = GenerateCrosshairMesh();
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    MaterialPropertyBlock m_BlackColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 0.0f, 1.0f});
    MaterialPropertyBlock m_BlueColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 1.0f, 1.0f});
    MaterialPropertyBlock m_RedColorMaterialProps = GeneratePropertyBlock({1.0f, 0.0f, 0.0f, 1.0f});

    // scene state
    std::vector<SceneSphere> m_SceneSpheres = GenerateSceneSpheres();
    AABB m_SceneSphereAABB = m_SphereMesh.getBounds();
    Sphere m_SceneSphereBoundingSphere = BoundingSphereOf(m_SphereMesh.getVerts());
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsShowingAABBs = true;
};


// public API

osc::CStringView osc::HittestTab::id() noexcept
{
    return c_TabStringID;
}

osc::HittestTab::HittestTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::HittestTab::HittestTab(HittestTab&&) noexcept = default;
osc::HittestTab& osc::HittestTab::operator=(HittestTab&&) noexcept = default;
osc::HittestTab::~HittestTab() noexcept = default;

osc::UID osc::HittestTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::HittestTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::HittestTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::HittestTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::HittestTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::HittestTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::HittestTab::implOnDraw()
{
    m_Impl->onDraw();
}
