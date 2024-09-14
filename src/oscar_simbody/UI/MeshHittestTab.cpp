#include "MeshHittestTab.h"

#include <oscar/Graphics/Geometries/AABBGeometry.h>
#include <oscar/Graphics/Geometries/SphereGeometry.h>
#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
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
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/Utils/UID.h>
#include <oscar_simbody/SimTKMeshLoader.h>

#include <array>
#include <cinttypes>
#include <chrono>

using namespace osc;

class osc::MeshHittestTab::Impl final {
public:

    Impl()
    {
        m_Camera.set_background_color(Color::white());
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return OSC_ICON_COOKIE " MeshHittestTab";
    }

    void on_tick()
    {
        ui::update_polar_camera_from_mouse_inputs(m_PolarCamera, App::get().main_window_dimensions());

        // handle hittest
        auto raycastStart = std::chrono::high_resolution_clock::now();

        Rect r = ui::get_main_viewport_workspace_uiscreenspace_rect();
        Vec2 d = dimensions_of(r);
        m_Ray = m_PolarCamera.unproject_topleft_pos_to_world_ray(Vec2{ui::get_mouse_pos()} - r.p1, d);

        m_IsMousedOver = false;
        if (m_UseBVH)
        {
            m_MeshBVH.for_each_ray_aabb_collision(m_Ray, [this](const BVHCollision& aabbColl)
            {
                const Triangle triangle = m_Mesh.get_triangle_at(aabbColl.id);
                if (auto triangleColl = find_collision(m_Ray, triangle))
                {
                    m_IsMousedOver = true;
                    m_Tris = triangle;
                }
            });
        }
        else
        {
            m_Mesh.for_each_indexed_triangle([this](Triangle triangle)
            {
                if (const auto hit = find_collision(m_Ray, triangle))
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
            const Rect viewportScreenRect = ui::get_main_viewport_workspace_screenspace_rect();
            m_Camera.set_pixel_rect(viewportScreenRect);

            // update real scene camera from constrained polar camera
            m_Camera.set_position(m_PolarCamera.position());
            m_Camera.set_clipping_planes({m_PolarCamera.znear, m_PolarCamera.zfar});
            m_Camera.set_view_matrix_override(m_PolarCamera.view_matrix());
            m_Camera.set_projection_matrix_override(m_PolarCamera.projection_matrix(aspect_ratio_of(viewportScreenRect)));
        }

        // draw mesh
        m_Material.set_color(m_IsMousedOver ? Color::green() : Color::red());
        m_Material.set_depth_tested(true);
        graphics::draw(m_Mesh, identity<Transform>(), m_Material, m_Camera);

        // draw hit triangle while mousing over
        if (m_IsMousedOver)
        {
            Mesh m;
            m.set_vertices(m_Tris);
            m.set_indices({0, 1, 2});

            m_Material.set_color(Color::black());
            m_Material.set_depth_tested(false);
            graphics::draw(m, identity<Transform>(), m_Material, m_Camera);
        }

        if (m_UseBVH)
        {
            // draw BVH AABBs
            m_Material.set_color(Color::black());
            m_Material.set_depth_tested(true);
            draw_bvh(
                *App::singleton<SceneCache>(),
                m_MeshBVH,
                [this](SceneDecoration&& dec)
                {
                    graphics::draw(m_CubeLinesMesh, dec.transform, m_Material, m_Camera);
                }
            );
        }

        // draw scene onto viewport
        m_Camera.render_to_screen();

        // auxiliary 2D UI
        // printout stats
        {
            ui::begin_panel("controls");
            ui::draw_checkbox("BVH", &m_UseBVH);
            ui::draw_text("%" PRId64 " microseconds", static_cast<int64_t>(m_RaycastDuration.count()));
            auto r = m_Ray;
            ui::draw_text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.position().x, m_Camera.position().y, m_Camera.position().z);
            ui::draw_text("origin = (%.2f, %.2f, %.2f), direction = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.direction.x, r.direction.y, r.direction.z);
            if (m_IsMousedOver)
            {
                ui::draw_text("hit = (%.2f, %.2f, %.2f)", m_HitPos.x, m_HitPos.y, m_HitPos.z);
                ui::draw_text("p0 = (%.2f, %.2f, %.2f)", m_Tris.p0.x, m_Tris.p0.y, m_Tris.p0.z);
                ui::draw_text("p1 = (%.2f, %.2f, %.2f)", m_Tris.p1.x, m_Tris.p1.y, m_Tris.p1.z);
                ui::draw_text("p2 = (%.2f, %.2f, %.2f)", m_Tris.p2.x, m_Tris.p2.y, m_Tris.p2.z);

            }
            ui::end_panel();
        }
        m_PerfPanel.on_draw();
    }

private:

    UID m_TabID;

    // rendering
    Camera m_Camera;

    MeshBasicMaterial m_Material;
    Mesh m_Mesh = LoadMeshViaSimTK(App::resource_filepath("geometry/hat_ribs.vtp"));
    Mesh m_SphereMesh = SphereGeometry{{.num_width_segments = 12, .num_height_segments = 12}};
    Mesh m_CubeLinesMesh = AABBGeometry{};

    // other state
    BVH m_MeshBVH = create_triangle_bvh(m_Mesh);
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

osc::MeshHittestTab::MeshHittestTab(const ParentPtr<ITabHost>&) :
    m_Impl{std::make_unique<Impl>()}
{}
osc::MeshHittestTab::MeshHittestTab(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab& osc::MeshHittestTab::operator=(MeshHittestTab&&) noexcept = default;
osc::MeshHittestTab::~MeshHittestTab() noexcept = default;

UID osc::MeshHittestTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::MeshHittestTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::MeshHittestTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::MeshHittestTab::impl_on_draw()
{
    m_Impl->onDraw();
}
