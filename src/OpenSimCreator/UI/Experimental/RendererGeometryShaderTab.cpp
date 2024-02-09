#include "RendererGeometryShaderTab.h"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>

#include <SDL_events.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

class osc::RendererGeometryShaderTab::Impl final {
public:

    Impl()
    {
        m_SceneCamera.setPosition({0.0f, 0.0f, 3.0f});
        m_SceneCamera.setVerticalFOV(45_deg);
        m_SceneCamera.setNearClippingPlane(0.1f);
        m_SceneCamera.setFarClippingPlane(100.0f);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "GeometryShader";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onDraw()
    {
        // handle mouse capturing
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_SceneCamera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }
        m_SceneCamera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        m_SceneMaterial.setColor("uDiffuseColor", m_MeshColor);
        Graphics::DrawMesh(m_Mesh, Transform{}, m_SceneMaterial, m_SceneCamera);
        Graphics::DrawMesh(m_Mesh, Transform{}, m_NormalsMaterial, m_SceneCamera);
        m_SceneCamera.renderToScreen();
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

    Mesh m_Mesh = LoadMeshViaSimTK(App::resourceFilepath("geometry/hat_ribs_scap.vtp"));
    Camera m_SceneCamera;
    bool m_IsMouseCaptured = false;
    Eulers m_CameraEulers = {};
    Color m_MeshColor = Color::white();
};


// public API (PIMPL)

CStringView osc::RendererGeometryShaderTab::id()
{
    return "OpenSim/Experimental/GeometryShader";
}

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab& osc::RendererGeometryShaderTab::operator=(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab::~RendererGeometryShaderTab() noexcept = default;

UID osc::RendererGeometryShaderTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::RendererGeometryShaderTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererGeometryShaderTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererGeometryShaderTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererGeometryShaderTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererGeometryShaderTab::implOnDraw()
{
    m_Impl->onDraw();
}
