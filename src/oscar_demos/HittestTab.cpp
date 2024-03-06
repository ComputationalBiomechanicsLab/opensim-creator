#include "HittestTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/Hittest";

    constexpr auto c_TriangleVerts = std::to_array<Vec3>({
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    });

    struct SceneSphere final {

        explicit SceneSphere(Vec3 pos_) :
            pos{pos_}
        {}

        Vec3 pos;
        bool isHovered = false;
    };

    std::vector<SceneSphere> GenerateSceneSpheres()
    {
        constexpr int32_t min = -30;
        constexpr int32_t max = 30;
        constexpr int32_t step = 6;

        std::vector<SceneSphere> rv;
        for (int32_t x = min; x <= max; x += step) {
            for (int32_t y = min; y <= max; y += step) {
                for (int32_t z = min; z <= max; z += step) {
                    rv.emplace_back(Vec3{
                        static_cast<float>(x),
                        50.0f + 2.0f*static_cast<float>(y),
                        static_cast<float>(z),
                    });
                }
            }
        }
        return rv;
    }

    Mesh GenerateCrosshairMesh()
    {
        Mesh rv;
        rv.setTopology(MeshTopology::Lines);
        rv.setVerts({
            // -X to +X
            {-0.05f, 0.0f, 0.0f},
            {+0.05f, 0.0f, 0.0f},

            // -Y to +Y
            {0.0f, -0.05f, 0.0f},
            {0.0f, +0.05f, 0.0f},
        });
        rv.setIndices({0, 1, 2, 3});
        return rv;
    }

    Mesh GenerateTriangleMesh()
    {
        Mesh rv;
        rv.setVerts(c_TriangleVerts);
        rv.setIndices({0, 1, 2});
        return rv;
    }

    MaterialPropertyBlock GeneratePropertyBlock(Color const& color)
    {
        MaterialPropertyBlock p;
        p.setColor("uColor", color);
        return p;
    }

    Line GetCameraRay(Camera const& camera)
    {
        return {
            camera.getPosition(),
            camera.getDirection(),
        };
    }
}

class osc::HittestTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
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
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && ui::IsMouseInMainViewportWorkspaceScreenRect()) {
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

        for (SceneSphere& ss : m_SceneSpheres) {
            ss.isHovered = false;

            Sphere s{
                ss.pos,
                m_SceneSphereBoundingSphere.radius
            };

            std::optional<RayCollision> res = find_collision(ray, s);
            if (res && res->distance >= 0.0f && res->distance < closestEl) {
                closestEl = res->distance;
                closestSceneSphere = &ss;
            }
        }

        if (closestSceneSphere) {
            closestSceneSphere->isHovered = true;
        }
    }

    void implOnDraw() final
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            ui::UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else {
            ui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // render spheres
        for (SceneSphere const& sphere : m_SceneSpheres) {

            Graphics::DrawMesh(
                m_SphereMesh,
                {.position = sphere.pos},
                m_Material,
                m_Camera,
                sphere.isHovered ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );

            // draw sphere AABBs
            if (m_IsShowingAABBs) {

                Graphics::DrawMesh(
                    m_WireframeCubeMesh,
                    {.scale = half_widths(m_SceneSphereAABB), .position = sphere.pos},
                    m_Material,
                    m_Camera,
                    m_BlackColorMaterialProps
                );
            }
        }

        // hittest + draw disc
        {
            Line const ray = GetCameraRay(m_Camera);

            Disc const sceneDisc{
                .origin = {0.0f, 0.0f, 0.0f},
                .normal = {0.0f, 1.0f, 0.0f},
                .radius = 10.0f,
            };

            std::optional<RayCollision> const maybeCollision = find_collision(ray, sceneDisc);

            Disc const meshDisc{
                .origin = {0.0f, 0.0f, 0.0f},
                .normal = {0.0f, 0.0f, 1.0f},
                .radius = 1.0f,
            };

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
            Line const ray = GetCameraRay(m_Camera);
            std::optional<RayCollision> const maybeCollision = find_collision(
                ray,
                Triangle{c_TriangleVerts.at(0), c_TriangleVerts.at(1), c_TriangleVerts.at(2)}
            );

            Graphics::DrawMesh(
                m_TriangleMesh,
                identity<Transform>(),
                m_Material,
                m_Camera,
                maybeCollision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );
        }

        Rect const viewport = ui::GetMainViewportWorkspaceScreenRect();

        // draw crosshair overlay
        Graphics::DrawMesh(
            m_CrosshairMesh,
            m_Camera.getInverseViewProjectionMatrix(AspectRatio(viewport)),
            m_Material,
            m_Camera,
            m_BlackColorMaterialProps
        );

        // draw scene to screen
        m_Camera.setPixelRect(viewport);
        m_Camera.renderToScreen();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Camera m_Camera;
    Material m_Material{Shader{
        m_Loader.slurp("oscar_demos/shaders/SolidColor.vert"),
        m_Loader.slurp("oscar_demos/shaders/SolidColor.frag"),
    }};
    Mesh m_SphereMesh = GenerateSphereMesh(1.0f, 12, 12);
    Mesh m_WireframeCubeMesh = GenerateCubeLinesMesh();
    Mesh m_CircleMesh = GenerateCircleMesh(1.0f, 36);
    Mesh m_CrosshairMesh = GenerateCrosshairMesh();
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    MaterialPropertyBlock m_BlackColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 0.0f, 1.0f});
    MaterialPropertyBlock m_BlueColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 1.0f, 1.0f});
    MaterialPropertyBlock m_RedColorMaterialProps = GeneratePropertyBlock({1.0f, 0.0f, 0.0f, 1.0f});

    // scene state
    std::vector<SceneSphere> m_SceneSpheres = GenerateSceneSpheres();
    AABB m_SceneSphereAABB = m_SphereMesh.getBounds();
    Sphere m_SceneSphereBoundingSphere = BoundingSphereOf(m_SphereMesh);
    bool m_IsMouseCaptured = false;
    Eulers m_CameraEulers{};
    bool m_IsShowingAABBs = true;
};


// public API

CStringView osc::HittestTab::id()
{
    return c_TabStringID;
}

osc::HittestTab::HittestTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::HittestTab::HittestTab(HittestTab&&) noexcept = default;
osc::HittestTab& osc::HittestTab::operator=(HittestTab&&) noexcept = default;
osc::HittestTab::~HittestTab() noexcept = default;

UID osc::HittestTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::HittestTab::implGetName() const
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
