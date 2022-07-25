#include "RendererBasicLightingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_events.h>

#include <utility>

// generate a texture-mapped cube
static osc::experimental::Mesh GenerateMesh()
{
    osc::MeshData cube = osc::GenCube();

    for (glm::vec3& vert : cube.verts)
    {
        vert *= 0.5f;  // makes the verts match LearnOpenGL
    }

    return osc::experimental::LoadMeshFromMeshData(cube);
}

class osc::RendererBasicLightingTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{parent}
    {
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_LightTransform.position = {1.2f, 1.0f, 2.0f};
        m_LightTransform.scale *= 0.2f;
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Basic Lighting (LearnOpenGL)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
    }

    void onMount()
    {
        osc::App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        osc::App::upd().setShowCursor(true);
        osc::App::upd().makeMainEventLoopWaiting();
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
        osc::App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // draw cube
        m_LightingMaterial.setVec3("uObjectColor", m_ObjectColor);
        m_LightingMaterial.setVec3("uLightColor", m_LightColor);
        m_LightingMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMaterial.setFloat("uAmbientStrength", m_AmbientStrength);
        m_LightingMaterial.setFloat("uDiffuseStrength", m_DiffuseStrength);
        m_LightingMaterial.setFloat("uSpecularStrength", m_SpecularStrength);
        osc::experimental::Graphics::DrawMesh(m_CubeMesh, osc::Transform{}, m_LightingMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setVec3("uLightColor", m_LightColor);
        osc::experimental::Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render to output (window)
        m_Camera.render();

        // render auxiliary UI
        ImGui::Begin("controls");
        ImGui::InputFloat3("light pos", glm::value_ptr(m_LightTransform.position));
        ImGui::InputFloat("ambient strength", &m_AmbientStrength);
        ImGui::InputFloat("diffuse strength", &m_DiffuseStrength);
        ImGui::InputFloat("specular strength", &m_SpecularStrength);
        ImGui::ColorEdit3("object color", glm::value_ptr(m_ObjectColor));
        ImGui::ColorEdit3("light color", glm::value_ptr(m_LightColor));
        ImGui::End();
    }

private:
    UID m_ID;
    TabHost* m_Parent;

    experimental::Shader m_LightingShader
    {
        osc::App::get().slurpResource("shaders/ExperimentBasicLighting.vert").c_str(),
        osc::App::get().slurpResource("shaders/ExperimentBasicLighting.frag").c_str(),
    };
    experimental::Material m_LightingMaterial{m_LightingShader};
    experimental::Shader m_LightCubeShader
    {
        osc::App::get().slurpResource("shaders/ExperimentLightCube.vert").c_str(),
        osc::App::get().slurpResource("shaders/ExperimentLightCube.frag").c_str(),
    };
    experimental::Material m_LightCubeMaterial{m_LightCubeShader};

    experimental::Mesh m_CubeMesh = GenerateMesh();

    experimental::Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform;
    glm::vec3 m_ObjectColor = {1.0f, 0.5f, 0.31f};
    glm::vec3 m_LightColor = {1.0f, 1.0f, 1.0f};
    float m_AmbientStrength = 0.1f;
    float m_DiffuseStrength = 1.0f;
    float m_SpecularStrength = 0.5f;
};


// public API (PIMPL)

osc::RendererBasicLightingTab::RendererBasicLightingTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererBasicLightingTab::RendererBasicLightingTab(RendererBasicLightingTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererBasicLightingTab& osc::RendererBasicLightingTab::operator=(RendererBasicLightingTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererBasicLightingTab::~RendererBasicLightingTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererBasicLightingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererBasicLightingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererBasicLightingTab::implParent() const
{
    return m_Impl->getParent();
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

void osc::RendererBasicLightingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererBasicLightingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererBasicLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
