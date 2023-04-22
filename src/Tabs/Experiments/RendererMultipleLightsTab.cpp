#include "RendererMultipleLightsTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Panels/LogViewerPanel.hpp"
#include "src/Panels/PerfPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <memory>

// positions of cubes within the scene
static glm::vec3 constexpr c_CubePositions[] =
{
    { 0.0f,  0.0f,  0.0f },
    { 2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f, -2.5f },
    {-3.8f, -2.0f, -12.3f},
    { 2.4f, -0.4f, -3.5f },
    {-1.7f,  3.0f, -7.5f },
    { 1.3f, -2.0f, -2.5f },
    { 1.5f,  2.0f, -2.5f },
    { 1.5f,  0.2f, -1.5f },
    {-1.3f,  1.0f, -1.5  },
};

// positions of point lights within the scene (the camera also has a spotlight)
static glm::vec3 constexpr c_PointLightPositions[] =
{
    { 0.7f,  0.2f,  2.0f },
    { 2.3f, -3.3f, -4.0f },
    {-4.0f,  2.0f, -12.0f},
    { 0.0f,  0.0f, -3.0f },
};

// ambient color of the point lights
static float constexpr c_PointLightAmbients[] =
{
    0.05f,
    0.05f,
    0.05f,
    0.05f,
};

// diffuse color of the point lights
static float constexpr c_PointLightDiffuses[] =
{
    0.8f,
    0.8f,
    0.8f,
    0.8f,
};

// specular color of the point lights
static float constexpr c_PointLightSpeculars[] =
{
    1.0f,
    1.0f,
    1.0f,
    1.0f,
};

static float constexpr c_PointLightConstants[] = {1.0f, 1.0f, 1.0f, 1.0f};
static float constexpr c_PointLightLinears[] = {0.09f, 0.09f, 0.09f, 0.09f};
static float constexpr c_PointLightQuadratics[] = {0.032f, 0.032f, 0.032f, 0.032f};

class osc::RendererMultipleLightsTab::Impl final {
public:

    Impl()
    {
        m_MultipleLightsMaterial.setTexture("uMaterialDiffuse", m_DiffuseMap);
        m_MultipleLightsMaterial.setTexture("uMaterialSpecular", m_SpecularMap);
        m_MultipleLightsMaterial.setVec3("uDirLightDirection", {-0.2f, -1.0f, -0.3f});
        m_MultipleLightsMaterial.setFloat("uDirLightAmbient", 0.05f);
        m_MultipleLightsMaterial.setFloat("uDirLightDiffuse", 0.4f);
        m_MultipleLightsMaterial.setFloat("uDirLightSpecular", 0.5f);

        m_MultipleLightsMaterial.setFloat("uSpotLightAmbient", 0.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightDiffuse", 1.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightSpecular", 1.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightConstant", 1.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightLinear", 0.09f);
        m_MultipleLightsMaterial.setFloat("uSpotLightQuadratic", 0.032f);
        m_MultipleLightsMaterial.setFloat("uSpotLightCutoff", glm::cos(glm::radians(12.5f)));
        m_MultipleLightsMaterial.setFloat("uSpotLightOuterCutoff", glm::cos(glm::radians(15.0f)));

        m_MultipleLightsMaterial.setVec3Array("uPointLightPos", c_PointLightPositions);
        m_MultipleLightsMaterial.setFloatArray("uPointLightConstant", c_PointLightConstants);
        m_MultipleLightsMaterial.setFloatArray("uPointLightLinear", c_PointLightLinears);
        m_MultipleLightsMaterial.setFloatArray("uPointLightQuadratic", c_PointLightQuadratics);
        m_MultipleLightsMaterial.setFloatArray("uPointLightAmbient", c_PointLightAmbients);
        m_MultipleLightsMaterial.setFloatArray("uPointLightDiffuse", c_PointLightDiffuses);
        m_MultipleLightsMaterial.setFloatArray("uPointLightSpecular", c_PointLightSpeculars);

        m_LightCubeMaterial.setColor("uLightColor", Color::white());

        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});

        m_LogViewer.open();
        m_PerfPanel.open();
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "Multiple Lights (LearnOpenGL)";
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

        // setup per-frame material vals
        m_MultipleLightsMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_MultipleLightsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        m_MultipleLightsMaterial.setVec3("uSpotLightPosition", m_Camera.getPosition());
        m_MultipleLightsMaterial.setVec3("uSpotLightDirection", m_Camera.getDirection());

        // render containers
        int32_t i = 0;
        glm::vec3 const axis = glm::normalize(glm::vec3{1.0f, 0.3f, 0.5f});
        for (glm::vec3 const& pos : c_CubePositions)
        {
            float const angle = glm::radians(i++ * 20.0f);

            Transform t;
            t.rotation = glm::angleAxis(angle, axis);
            t.position = pos;

            Graphics::DrawMesh(m_Mesh, t, m_MultipleLightsMaterial, m_Camera);
        }

        // render lamps
        Transform lampXform;
        lampXform.scale = {0.2f, 0.2f, 0.2f};
        for (glm::vec3 const& pos : c_PointLightPositions)
        {
            lampXform.position = pos;
            Graphics::DrawMesh(m_Mesh, lampXform, m_LightCubeMaterial, m_Camera);
        }

        // render to output (window)
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        // render auxiliary UI
        ImGui::Begin("controls");
        ImGui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ImGui::End();

        m_LogViewer.draw();
        m_PerfPanel.draw();
    }

private:
    UID m_TabID;

    Material m_MultipleLightsMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentMultipleLights.vert"),
            App::slurp("shaders/ExperimentMultipleLights.frag"),
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
    Mesh m_Mesh = GenLearnOpenGLCube();
    Texture2D m_DiffuseMap = LoadTexture2DFromImage(App::resource("textures/container2.png"), ImageFlags_FlipVertically);
    Texture2D m_SpecularMap = LoadTexture2DFromImage(App::resource("textures/container2_specular.png"), ImageFlags_FlipVertically);

    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    float m_MaterialShininess = 16.0f;

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::CStringView osc::RendererMultipleLightsTab::id() noexcept
{
    return "Renderer/MultipleLights";
}

osc::RendererMultipleLightsTab::RendererMultipleLightsTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererMultipleLightsTab::RendererMultipleLightsTab(RendererMultipleLightsTab&&) noexcept = default;
osc::RendererMultipleLightsTab& osc::RendererMultipleLightsTab::operator=(RendererMultipleLightsTab&&) noexcept = default;
osc::RendererMultipleLightsTab::~RendererMultipleLightsTab() noexcept = default;

osc::UID osc::RendererMultipleLightsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererMultipleLightsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererMultipleLightsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererMultipleLightsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererMultipleLightsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererMultipleLightsTab::implOnDraw()
{
    m_Impl->onDraw();
}
