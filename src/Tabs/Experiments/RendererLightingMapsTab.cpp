#include "RendererLightingMapsTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <glm/gtc/type_ptr.hpp>

#include <utility>

class osc::RendererLightingMapsTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_LightingMapsMaterial.setTexture("uMaterialDiffuse", m_DiffuseMap);
        m_LightingMapsMaterial.setTexture("uMaterialSpecular", m_SpecularMap);
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_LightTransform.position = {0.4f, 0.4f, 2.0f};
        m_LightTransform.scale = {0.2f, 0.2f, 0.2f};
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Lighting Maps (LearnOpenGL)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
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

    void onTick()
    {
    }

    void onDrawMainMenu()
    {
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
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // draw cube
        m_LightingMapsMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMapsMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMapsMaterial.setVec3("uLightAmbient", m_LightAmbient);
        m_LightingMapsMaterial.setVec3("uLightDiffuse", m_LightDiffuse);
        m_LightingMapsMaterial.setVec3("uLightSpecular", m_LightSpecular);
        m_LightingMapsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        experimental::Graphics::DrawMesh(m_Mesh, Transform{}, m_LightingMapsMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setVec3("uLightColor", {1.0f, 1.0f, 1.0f});
        osc::experimental::Graphics::DrawMesh(m_Mesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render to output (window)
        m_Camera.render();

        // render auxiliary UI
        ImGui::Begin("controls");
        ImGui::InputFloat3("uLightPos", glm::value_ptr(m_LightTransform.position));
        ImGui::InputFloat3("uLightAmbient", glm::value_ptr(m_LightAmbient));
        ImGui::InputFloat3("uLightDiffuse", glm::value_ptr(m_LightDiffuse));
        ImGui::InputFloat3("uLightSpecular", glm::value_ptr(m_LightSpecular));
        ImGui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ImGui::End();
    }

private:
    UID m_ID;
    TabHost* m_Parent;

    Material m_LightingMapsMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightingMaps.vert"),
            App::slurp("shaders/ExperimentLightingMaps.frag"),
        }
    };
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightCube.vert"),
            App::slurp("shaders/ExperimentLightCube.frag"),
        }
    };
    experimental::Mesh m_Mesh = GenLearnOpenGLCube();
    Texture2D m_DiffuseMap = LoadTexture2DFromImageResource("textures/container2.png", ImageFlags_FlipVertically);
    Texture2D m_SpecularMap = LoadTexture2DFromImageResource("textures/container2_specular.png", ImageFlags_FlipVertically);

    experimental::Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform;
    glm::vec3 m_LightAmbient = {0.2f, 0.2f, 0.2f};
    glm::vec3 m_LightDiffuse = {0.5f, 0.5f, 0.5f};
    glm::vec3 m_LightSpecular = {1.0f, 1.0f, 1.0f};

    float m_MaterialShininess = 64.0f;
};


// public API (PIMPL)

osc::RendererLightingMapsTab::RendererLightingMapsTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererLightingMapsTab::RendererLightingMapsTab(RendererLightingMapsTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererLightingMapsTab& osc::RendererLightingMapsTab::operator=(RendererLightingMapsTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererLightingMapsTab::~RendererLightingMapsTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererLightingMapsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererLightingMapsTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererLightingMapsTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererLightingMapsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererLightingMapsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererLightingMapsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererLightingMapsTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererLightingMapsTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererLightingMapsTab::implOnDraw()
{
    m_Impl->onDraw();
}
