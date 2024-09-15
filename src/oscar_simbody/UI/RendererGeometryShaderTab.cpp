#include "RendererGeometryShaderTab.h"

#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Event.h>
#include <oscar/UI/oscimgui.h>
#include <oscar_simbody/SimTKMeshLoader.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

class osc::RendererGeometryShaderTab::Impl final {
public:

    Impl()
    {
        m_SceneCamera.set_position({0.0f, 0.0f, 3.0f});
        m_SceneCamera.set_vertical_fov(45_deg);
        m_SceneCamera.set_clipping_planes({0.1f, 100.0f});
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "GeometryShader";
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        m_IsMouseCaptured = true;
    }

    void on_unmount()
    {
        m_IsMouseCaptured = false;
        App::upd().set_show_cursor(true);
        App::upd().make_main_loop_waiting();
    }

    bool onEvent(const SDL_Event& e)
    {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && ui::is_mouse_in_main_viewport_workspace())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onDraw()
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            ui::update_camera_from_all_inputs(m_SceneCamera, m_CameraEulers);
            ui::hide_mouse_cursor();
            App::upd().set_show_cursor(false);
        }
        else {
            ui::show_mouse_cursor();
            App::upd().set_show_cursor(true);
        }
        m_SceneCamera.set_pixel_rect(ui::get_main_viewport_workspace_screenspace_rect());

        m_SceneMaterial.set("uDiffuseColor", m_MeshColor);
        graphics::draw(m_Mesh, identity<Transform>(), m_SceneMaterial, m_SceneCamera);
        graphics::draw(m_Mesh, identity<Transform>(), m_NormalsMaterial, m_SceneCamera);
        m_SceneCamera.render_to_screen();
    }

private:
    UID m_TabID;

    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("shaders/GeometryShaderTab/Scene.vert"),
            App::slurp("shaders/GeometryShaderTab/Scene.frag"),
        },
    };

    Material m_NormalsMaterial
    {
        Shader
        {
            App::slurp("shaders/GeometryShaderTab/DrawNormals.vert"),
            App::slurp("shaders/GeometryShaderTab/DrawNormals.geom"),
            App::slurp("shaders/GeometryShaderTab/DrawNormals.frag"),
        },
    };

    Mesh m_Mesh = LoadMeshViaSimTK(App::resource_filepath("geometry/hat_ribs_scap.vtp"));
    Camera m_SceneCamera;
    bool m_IsMouseCaptured = false;
    EulerAngles m_CameraEulers = {};
    Color m_MeshColor = Color::white();
};


CStringView osc::RendererGeometryShaderTab::id()
{
    return "OpenSim/Experimental/GeometryShader";
}

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(const ParentPtr<ITabHost>&) :
    m_Impl{std::make_unique<Impl>()}
{}
osc::RendererGeometryShaderTab::RendererGeometryShaderTab(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab& osc::RendererGeometryShaderTab::operator=(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab::~RendererGeometryShaderTab() noexcept = default;

UID osc::RendererGeometryShaderTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::RendererGeometryShaderTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::RendererGeometryShaderTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::RendererGeometryShaderTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::RendererGeometryShaderTab::impl_on_event(const Event& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererGeometryShaderTab::impl_on_draw()
{
    m_Impl->onDraw();
}
