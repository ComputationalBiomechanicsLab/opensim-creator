#include "MeshHittestTab.h"

#include <libopensimcreator/Graphics/SimTKMeshLoader.h>
#include <libopensimcreator/Platform/IconCodepoints.h>

#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Geometries/AABBGeometry.h>
#include <liboscar/Graphics/Geometries/SphereGeometry.h>
#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Materials/MeshBasicMaterial.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/BVH.h>
#include <liboscar/Maths/CollisionTests.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Ray.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Triangle.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/PerfPanel.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/PerfClock.h>

#include <cinttypes>
#include <chrono>

using namespace osc;

class osc::MeshHittestTab::Impl final : public TabPrivate {
public:

    explicit Impl(MeshHittestTab& owner, Widget* parent) :
        TabPrivate{owner, parent, OSC_ICON_COOKIE " MeshHittestTab"}
    {
        m_Camera.set_background_color(Color::white());
    }

    void on_tick()
    {
        ui::update_polar_camera_from_mouse_inputs(m_PolarCamera, App::get().main_window_dimensions());

        // handle hittest
        const auto raycastStartTime = PerfClock::now();

        const Rect r = ui::get_main_window_workspace_ui_rect();
        const Vec2 d = r.dimensions();
        m_Ray = m_PolarCamera.unproject_topleft_position_to_world_ray(ui::get_mouse_ui_position() - r.ypd_top_left(), d);

        m_IsMousedOver = false;
        if (m_UseBVH) {
            m_MeshBVH.for_each_ray_aabb_collision(m_Ray, [this](const BVHCollision& aabbColl)
            {
                const Triangle triangle = m_Mesh.get_triangle_at(aabbColl.id);
                if (auto triangleColl = find_collision(m_Ray, triangle)) {
                    m_IsMousedOver = true;
                    m_Tris = triangle;
                }
            });
        }
        else {
            m_Mesh.for_each_indexed_triangle([this](Triangle triangle)
            {
                if (const auto hit = find_collision(m_Ray, triangle)) {
                    m_HitPos = hit->position;
                    m_IsMousedOver = true;
                    m_Tris = triangle;
                }
            });
        }

        const auto raycastEndTime = PerfClock::now();
        m_RaycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastEndTime - raycastStartTime);
    }

    void onDraw()
    {
        // setup scene
        {
            const Rect workspaceRect = ui::get_main_window_workspace_screen_space_rect();
            m_Camera.set_pixel_rect(workspaceRect);

            // update real scene camera from constrained polar camera
            m_Camera.set_position(m_PolarCamera.position());
            m_Camera.set_clipping_planes({m_PolarCamera.znear, m_PolarCamera.zfar});
            m_Camera.set_view_matrix_override(m_PolarCamera.view_matrix());
            m_Camera.set_projection_matrix_override(m_PolarCamera.projection_matrix(aspect_ratio_of(workspaceRect)));
        }

        // draw mesh
        m_Material.set_color(m_IsMousedOver ? Color::green() : Color::red());
        m_Material.set_depth_tested(true);
        graphics::draw(m_Mesh, identity<Transform>(), m_Material, m_Camera);

        // draw hit triangle while mousing over
        if (m_IsMousedOver) {
            Mesh m;
            m.set_vertices(m_Tris);
            m.set_indices({0, 1, 2});

            m_Material.set_color(Color::black());
            m_Material.set_depth_tested(false);
            graphics::draw(m, identity<Transform>(), m_Material, m_Camera);
        }

        if (m_UseBVH) {
            // draw BVH AABBs
            m_Material.set_color(Color::black());
            m_Material.set_depth_tested(true);
            draw_bvh(
                *App::singleton<SceneCache>(),
                m_MeshBVH,
                [this](const SceneDecoration& dec)
                {
                    graphics::draw(m_CubeLinesMesh, dec.transform, m_Material, m_Camera);
                }
            );
        }

        // draw scene onto workspace
        m_Camera.render_to_main_window();

        // auxiliary 2D UI
        // printout stats
        {
            ui::begin_panel("controls");
            ui::draw_checkbox("BVH", &m_UseBVH);
            ui::draw_text("%" PRId64 " microseconds", static_cast<int64_t>(m_RaycastDuration.count()));
            auto r = m_Ray;
            ui::draw_text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.position().x, m_Camera.position().y, m_Camera.position().z);
            ui::draw_text("origin = (%.2f, %.2f, %.2f), direction = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.direction.x, r.direction.y, r.direction.z);
            if (m_IsMousedOver) {
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
    // rendering
    Camera m_Camera;

    MeshBasicMaterial m_Material;
    Mesh m_Mesh = LoadMeshViaSimTK(App::resource_filepath("OpenSimCreator/geometry/hat_ribs.vtp"));
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
    Ray m_Ray{};

    PerfPanel m_PerfPanel{&owner()};
};


CStringView osc::MeshHittestTab::id() { return "OpenSim/MeshHittest"; }

osc::MeshHittestTab::MeshHittestTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::MeshHittestTab::impl_on_tick() { private_data().on_tick(); }
void osc::MeshHittestTab::impl_on_draw() { private_data().onDraw(); }
