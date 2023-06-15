#include "RendererGeometryShaderTab.hpp"

#include "OpenSimCreator/Bindings/SimTKMeshLoader.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Platform/App.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <utility>

class osc::RendererGeometryShaderTab::Impl final {
public:

    Impl()
    {
        m_SceneCamera.setPosition({0.0f, 0.0f, 3.0f});
        m_SceneCamera.setCameraFOV(glm::radians(45.0f));
        m_SceneCamera.setNearClippingPlane(0.1f);
        m_SceneCamera.setFarClippingPlane(100.0f);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "GeometryShader (LearnOpenGL)";
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
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
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
        m_SceneCamera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        m_SceneMaterial.setColor("uDiffuseColor", m_MeshColor);
        Graphics::DrawMesh(m_Mesh, osc::Transform{}, m_SceneMaterial, m_SceneCamera);
        Graphics::DrawMesh(m_Mesh, osc::Transform{}, m_NormalsMaterial, m_SceneCamera);
        m_SceneCamera.renderToScreen();
    }

private:
    UID m_TabID;

    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentGeometryShaderScene.vert"),
            App::slurp("shaders/ExperimentGeometryShaderScene.frag"),
        },
    };

    Material m_NormalsMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentGeometryShaderNormals.vert"),
            App::slurp("shaders/ExperimentGeometryShaderNormals.geom"),
            App::slurp("shaders/ExperimentGeometryShaderNormals.frag"),
        },
    };

    Mesh m_Mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs_scap.vtp"));
    Camera m_SceneCamera;
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Color m_MeshColor = Color::white();
};


// public API (PIMPL)

osc::CStringView osc::RendererGeometryShaderTab::id() noexcept
{
    return "OpenSim/Experimental/GeometryShader";
}

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererGeometryShaderTab::RendererGeometryShaderTab(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab& osc::RendererGeometryShaderTab::operator=(RendererGeometryShaderTab&&) noexcept = default;
osc::RendererGeometryShaderTab::~RendererGeometryShaderTab() noexcept = default;

osc::UID osc::RendererGeometryShaderTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererGeometryShaderTab::implGetName() const
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
