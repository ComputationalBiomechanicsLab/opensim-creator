#include "RendererGeometryShaderTab.h"

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/materials/mesh_basic_material.h>
#include <liboscar/graphics/materials/mesh_normal_vectors_material.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/cursor.h>
#include <liboscar/platform/cursor_shape.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

class osc::RendererGeometryShaderTab::Impl final : public TabPrivate {
public:

    explicit Impl(RendererGeometryShaderTab& owner, Widget* parent) :
        TabPrivate{owner, parent, "GeometryShader"}
    {
        m_SceneCamera.set_position({0.0f, 0.0f, 3.0f});
        m_SceneCamera.set_vertical_field_of_view(45_deg);
        m_SceneCamera.set_clipping_planes({0.1f, 100.0f});
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        grab_mouse(true);
    }

    void on_unmount()
    {
        grab_mouse(false);
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::KeyUp and dynamic_cast<const KeyEvent&>(e).combination() == Key::Escape) {
            grab_mouse(false);
            return true;
        }
        else if (e.type() == EventType::MouseButtonDown and ui::is_mouse_in_main_window_workspace()) {
            grab_mouse(true);
            return true;
        }
        return false;
    }

    void onDraw()
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            ui::update_camera_from_all_inputs(m_SceneCamera, m_CameraEulers);
        }
        m_SceneCamera.set_pixel_rect(ui::get_main_window_workspace_screen_space_rect());

        m_SceneMaterial.set("uDiffuseColor", m_MeshColor);
        graphics::draw(m_Mesh, identity<Transform>(), m_SceneMaterial, m_SceneCamera);
        graphics::draw(m_Mesh, identity<Transform>(), m_NormalsMaterial, m_SceneCamera);
        m_SceneCamera.render_to_main_window();
    }

private:
    void grab_mouse(bool v)
    {
        if (v) {
            if (not std::exchange(m_IsMouseCaptured, true)) {
                App::upd().push_cursor_override(Cursor{CursorShape::Hidden});
                App::upd().enable_main_window_grab();
            }
        }
        else {
            if (std::exchange(m_IsMouseCaptured, false)) {
                App::upd().disable_main_window_grab();
                App::upd().pop_cursor_override();
            }
        }
    }

    MeshBasicMaterial m_SceneMaterial;
    MeshNormalVectorsMaterial m_NormalsMaterial;

    Mesh m_Mesh = LoadMeshViaSimbody(App::resource_filepath("OpenSimCreator/geometry/hat_ribs_scap.vtp").value_or(std::filesystem::path{"OpenSimCreator/geometry/hat_ribs_scap.vtp"}));
    Camera m_SceneCamera;
    bool m_IsMouseCaptured = false;
    EulerAngles m_CameraEulers;
    Color m_MeshColor = Color::white();
};


CStringView osc::RendererGeometryShaderTab::id() { return "OpenSim/RendererGeometryShader"; }

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::RendererGeometryShaderTab::impl_on_mount() { private_data().on_mount(); }
void osc::RendererGeometryShaderTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::RendererGeometryShaderTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::RendererGeometryShaderTab::impl_on_draw() { private_data().onDraw(); }

