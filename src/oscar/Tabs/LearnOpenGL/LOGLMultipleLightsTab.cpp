#include "LOGLMultipleLightsTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Panels/LogViewerPanel.hpp"
#include "oscar/Panels/PerfPanel.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Platform/Log.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/MultipleLights";

    // positions of cubes within the scene
    constexpr auto c_CubePositions = osc::to_array<glm::vec3>(
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
    });

    // positions of point lights within the scene (the camera also has a spotlight)
    constexpr auto c_PointLightPositions = osc::to_array<glm::vec3>(
    {
        { 0.7f,  0.2f,  2.0f },
        { 2.3f, -3.3f, -4.0f },
        {-4.0f,  2.0f, -12.0f},
        { 0.0f,  0.0f, -3.0f },
    });
    constexpr auto c_PointLightAmbients = osc::to_array<float>({0.001f, 0.001f, 0.001f, 0.001f});
    constexpr auto c_PointLightDiffuses = osc::to_array<float>({0.2f, 0.2f, 0.2f, 0.2f});
    constexpr auto c_PointLightSpeculars = osc::to_array<float>({0.5f, 0.5f, 0.5f, 0.5f});
    constexpr auto c_PointLightConstants = osc::to_array<float>({1.0f, 1.0f, 1.0f, 1.0f});
    constexpr auto c_PointLightLinears = osc::to_array<float>({0.09f, 0.09f, 0.09f, 0.09f});
    constexpr auto c_PointLightQuadratics = osc::to_array<float>({0.032f, 0.032f, 0.032f, 0.032f});
}

class osc::LOGLMultipleLightsTab::Impl final {
public:

    Impl()
    {
        m_MultipleLightsMaterial.setTexture("uMaterialDiffuse", m_DiffuseMap);
        m_MultipleLightsMaterial.setTexture("uMaterialSpecular", m_SpecularMap);
        m_MultipleLightsMaterial.setVec3("uDirLightDirection", {-0.2f, -1.0f, -0.3f});
        m_MultipleLightsMaterial.setFloat("uDirLightAmbient", 0.01f);
        m_MultipleLightsMaterial.setFloat("uDirLightDiffuse", 0.2f);
        m_MultipleLightsMaterial.setFloat("uDirLightSpecular", 0.4f);

        m_MultipleLightsMaterial.setFloat("uSpotLightAmbient", 0.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightDiffuse", 1.0f);
        m_MultipleLightsMaterial.setFloat("uSpotLightSpecular", 0.75f);

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
    Texture2D m_DiffuseMap = LoadTexture2DFromImage(
        App::resource("textures/container2.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );
    Texture2D m_SpecularMap = LoadTexture2DFromImage(
        App::resource("textures/container2_specular.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );

    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    float m_MaterialShininess = 64.0f;

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API (PIMPL)

osc::CStringView osc::LOGLMultipleLightsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(LOGLMultipleLightsTab&&) noexcept = default;
osc::LOGLMultipleLightsTab& osc::LOGLMultipleLightsTab::operator=(LOGLMultipleLightsTab&&) noexcept = default;
osc::LOGLMultipleLightsTab::~LOGLMultipleLightsTab() noexcept = default;

osc::UID osc::LOGLMultipleLightsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLMultipleLightsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLMultipleLightsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLMultipleLightsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLMultipleLightsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLMultipleLightsTab::implOnDraw()
{
    m_Impl->onDraw();
}
