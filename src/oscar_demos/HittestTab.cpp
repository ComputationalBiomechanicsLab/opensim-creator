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
        rv.set_topology(MeshTopology::Lines);
        rv.set_vertices({
            // -X to +X
            {-0.05f, 0.0f, 0.0f},
            {+0.05f, 0.0f, 0.0f},

            // -Y to +Y
            {0.0f, -0.05f, 0.0f},
            {0.0f, +0.05f, 0.0f},
        });
        rv.set_indices({0, 1, 2, 3});
        return rv;
    }

    Mesh GenerateTriangleMesh()
    {
        Mesh rv;
        rv.set_vertices(c_TriangleVerts);
        rv.set_indices({0, 1, 2});
        return rv;
    }

    Line GetCameraRay(Camera const& camera)
    {
        return {
            camera.position(),
            camera.direction(),
        };
    }
}

class osc::HittestTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_Camera.set_background_color({1.0f, 1.0f, 1.0f, 0.0f});
    }

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        m_IsMouseCaptured = true;
    }

    void impl_on_unmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().make_main_loop_waiting();
        App::upd().set_show_cursor(true);
    }

    bool impl_on_event(SDL_Event const& e) final
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

    void impl_on_tick() final
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

    void impl_on_draw() final
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            ui::UpdateCameraFromInputs(m_Camera, m_CameraEulers);
            ui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().set_show_cursor(false);
        }
        else {
            ui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().set_show_cursor(true);
        }

        // render spheres
        for (SceneSphere const& sphere : m_SceneSpheres) {

            graphics::draw(
                m_SphereMesh,
                {.position = sphere.pos},
                m_Material,
                m_Camera,
                sphere.isHovered ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );

            // draw sphere AABBs
            if (m_IsShowingAABBs) {

                graphics::draw(
                    m_WireframeCubeMesh,
                    {.scale = half_widths_of(m_SceneSphereAABB), .position = sphere.pos},
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

            graphics::draw(
                m_CircleMesh,
                mat4_transform_between(meshDisc, sceneDisc),
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

            graphics::draw(
                m_TriangleMesh,
                identity<Transform>(),
                m_Material,
                m_Camera,
                maybeCollision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
            );
        }

        Rect const viewport = ui::GetMainViewportWorkspaceScreenRect();

        // draw crosshair overlay
        graphics::draw(
            m_CrosshairMesh,
            m_Camera.inverse_view_projection_matrix(aspect_ratio_of(viewport)),
            m_Material,
            m_Camera,
            m_BlackColorMaterialProps
        );

        // draw scene to screen
        m_Camera.set_pixel_rect(viewport);
        m_Camera.render_to_screen();
    }

    Camera m_Camera;
    MeshBasicMaterial m_Material;
    Mesh m_SphereMesh = SphereGeometry{1.0f, 12, 12};
    Mesh m_WireframeCubeMesh = AABBGeometry{};
    Mesh m_CircleMesh = CircleGeometry{1.0f, 36};
    Mesh m_CrosshairMesh = GenerateCrosshairMesh();
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    MeshBasicMaterial::PropertyBlock m_BlackColorMaterialProps{Color::black()};
    MeshBasicMaterial::PropertyBlock m_BlueColorMaterialProps{Color::blue()};
    MeshBasicMaterial::PropertyBlock m_RedColorMaterialProps{Color::red()};

    // scene state
    std::vector<SceneSphere> m_SceneSpheres = GenerateSceneSpheres();
    AABB m_SceneSphereAABB = m_SphereMesh.bounds();
    Sphere m_SceneSphereBoundingSphere = bounding_sphere_of(m_SphereMesh);
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

UID osc::HittestTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::HittestTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::HittestTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::HittestTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::HittestTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::HittestTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::HittestTab::impl_on_draw()
{
    m_Impl->on_draw();
}
