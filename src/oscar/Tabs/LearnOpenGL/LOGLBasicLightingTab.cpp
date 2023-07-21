#include "LOGLBasicLightingTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_events.h>

#include <memory>

namespace
{
    osc::CStringView constexpr c_TabStringID = "LearnOpenGL/BasicLighting";
}

class osc::LOGLBasicLightingTab::Impl final {
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
        return c_TabStringID;
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
    float m_AmbientStrength = 0.01f;
    float m_DiffuseStrength = 0.6f;
    float m_SpecularStrength = 1.0f;
};


// public API (PIMPL)

osc::CStringView osc::LOGLBasicLightingTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab& osc::LOGLBasicLightingTab::operator=(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab::~LOGLBasicLightingTab() noexcept = default;

osc::UID osc::LOGLBasicLightingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLBasicLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBasicLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBasicLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBasicLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBasicLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
