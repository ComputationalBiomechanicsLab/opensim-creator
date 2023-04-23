#include "RendererBasicLightingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_events.h>

#include <memory>

class osc::RendererBasicLightingTab::Impl final {
public:

    Impl()
    {
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        m_LightTransform.position = {1.2f, 1.0f, 2.0f};
        m_LightTransform.scale *= 0.2f;
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "Basic Lighting (LearnOpenGL)";
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
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // draw cube
        m_LightingMaterial.setColor("uObjectColor", m_ObjectColor);
        m_LightingMaterial.setColor("uLightColor", m_LightColor);
        m_LightingMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMaterial.setFloat("uAmbientStrength", m_AmbientStrength);
        m_LightingMaterial.setFloat("uDiffuseStrength", m_DiffuseStrength);
        m_LightingMaterial.setFloat("uSpecularStrength", m_SpecularStrength);
        Graphics::DrawMesh(m_CubeMesh, osc::Transform{}, m_LightingMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setColor("uLightColor", m_LightColor);
        Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render to output (window)
        m_Camera.renderToScreen();

        // render auxiliary UI
        ImGui::Begin("controls");
        ImGui::InputFloat3("light pos", glm::value_ptr(m_LightTransform.position));
        ImGui::InputFloat("ambient strength", &m_AmbientStrength);
        ImGui::InputFloat("diffuse strength", &m_DiffuseStrength);
        ImGui::InputFloat("specular strength", &m_SpecularStrength);
        ImGui::ColorEdit3("object color", ValuePtr(m_ObjectColor));
        ImGui::ColorEdit3("light color", ValuePtr(m_LightColor));
        ImGui::End();
    }

private:
    UID m_TabID;

    Material m_LightingMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentBasicLighting.vert"),
            App::slurp("shaders/ExperimentBasicLighting.frag"),
        },
    };
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightCube.vert"),
            App::slurp("shaders/ExperimentLightCube.frag"),
        },
    };

    Mesh m_CubeMesh = GenLearnOpenGLCube();

    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform;
    Color m_ObjectColor = {1.0f, 0.5f, 0.31f, 1.0f};
    Color m_LightColor = Color::white();
    float m_AmbientStrength = 0.1f;
    float m_DiffuseStrength = 1.0f;
    float m_SpecularStrength = 0.5f;
};


// public API (PIMPL)

osc::CStringView osc::RendererBasicLightingTab::id() noexcept
{
    return "Renderer/BasicLighting";
}

osc::RendererBasicLightingTab::RendererBasicLightingTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererBasicLightingTab::RendererBasicLightingTab(RendererBasicLightingTab&&) noexcept = default;
osc::RendererBasicLightingTab& osc::RendererBasicLightingTab::operator=(RendererBasicLightingTab&&) noexcept = default;
osc::RendererBasicLightingTab::~RendererBasicLightingTab() noexcept = default;

osc::UID osc::RendererBasicLightingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererBasicLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererBasicLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererBasicLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererBasicLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererBasicLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
