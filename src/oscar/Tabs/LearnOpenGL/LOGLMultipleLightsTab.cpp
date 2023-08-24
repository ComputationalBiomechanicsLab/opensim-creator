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
#include "oscar/Tabs/StandardTabBase.hpp"
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

    osc::Camera CreateCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    osc::Material CreateMultipleLightsMaterial()
    {
        osc::Texture2D diffuseMap = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/container2.png"),
            osc::ColorSpace::sRGB,
            osc::ImageLoadingFlags::FlipVertically
        );

        osc::Texture2D specularMap = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/container2_specular.png"),
            osc::ColorSpace::sRGB,
            osc::ImageLoadingFlags::FlipVertically
        );

        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentMultipleLights.vert"),
                osc::App::slurp("shaders/ExperimentMultipleLights.frag"),
            },
        };

        rv.setTexture("uMaterialDiffuse", diffuseMap);
        rv.setTexture("uMaterialSpecular", specularMap);
        rv.setVec3("uDirLightDirection", {-0.2f, -1.0f, -0.3f});
        rv.setFloat("uDirLightAmbient", 0.01f);
        rv.setFloat("uDirLightDiffuse", 0.2f);
        rv.setFloat("uDirLightSpecular", 0.4f);

        rv.setFloat("uSpotLightAmbient", 0.0f);
        rv.setFloat("uSpotLightDiffuse", 1.0f);
        rv.setFloat("uSpotLightSpecular", 0.75f);

        rv.setFloat("uSpotLightConstant", 1.0f);
        rv.setFloat("uSpotLightLinear", 0.09f);
        rv.setFloat("uSpotLightQuadratic", 0.032f);
        rv.setFloat("uSpotLightCutoff", glm::cos(glm::radians(12.5f)));
        rv.setFloat("uSpotLightOuterCutoff", glm::cos(glm::radians(15.0f)));

        rv.setVec3Array("uPointLightPos", c_PointLightPositions);
        rv.setFloatArray("uPointLightConstant", c_PointLightConstants);
        rv.setFloatArray("uPointLightLinear", c_PointLightLinears);
        rv.setFloatArray("uPointLightQuadratic", c_PointLightQuadratics);
        rv.setFloatArray("uPointLightAmbient", c_PointLightAmbients);
        rv.setFloatArray("uPointLightDiffuse", c_PointLightDiffuses);
        rv.setFloatArray("uPointLightSpecular", c_PointLightSpeculars);

        return rv;
    }

    osc::Material CreateLightCubeMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentLightCube.vert"),
                osc::App::slurp("shaders/ExperimentLightCube.frag"),
            },
        };
        rv.setColor("uLightColor", osc::Color::white());
        return rv;
    }
}

class osc::LOGLMultipleLightsTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        m_LogViewer.open();
        m_PerfPanel.open();
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
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

    void implOnDraw() final
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
            float const angle = glm::radians(static_cast<float>(i++) * 20.0f);

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

        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    Material m_MultipleLightsMaterial = CreateMultipleLightsMaterial();
    Material m_LightCubeMaterial = CreateLightCubeMaterial();
    Mesh m_Mesh = GenLearnOpenGLCube();

    Camera m_Camera = CreateCamera();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = false;

    float m_MaterialShininess = 64.0f;

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

osc::CStringView osc::LOGLMultipleLightsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(ParentPtr<TabHost> const&) :
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
