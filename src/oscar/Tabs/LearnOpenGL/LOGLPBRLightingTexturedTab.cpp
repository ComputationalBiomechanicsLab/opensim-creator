#include "LOGLPBRLightingTexturedTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Panels/PerfPanel.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/LightingTextured";

    constexpr auto c_LightPositions = osc::to_array<glm::vec3>(
    {
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<glm::vec3, c_LightPositions.size()> c_LightRadiances = osc::to_array<glm::vec3>(
    {
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_NumRows = 7;
    constexpr int c_NumCols = 7;
    constexpr float c_CellSpacing = 2.5f;

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

    osc::Material CreateMaterial()
    {
        osc::Texture2D albedo = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/pbr/rusted_iron/albedo.png"),
            osc::ColorSpace::sRGB
        );
        osc::Texture2D normal = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/pbr/rusted_iron/normal.png"),
            osc::ColorSpace::Linear
        );
        osc::Texture2D metallic = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/pbr/rusted_iron/metallic.png"),
            osc::ColorSpace::Linear
        );
        osc::Texture2D roughness = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/pbr/rusted_iron/roughness.png"),
            osc::ColorSpace::Linear
        );
        osc::Texture2D ao = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/pbr/rusted_iron/ao.png"),
            osc::ColorSpace::Linear
        );

        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentPBRLightingTextured.vert"),
                osc::App::slurp("shaders/ExperimentPBRLightingTextured.frag"),
            },
        };
        rv.setTexture("uAlbedoMap", albedo);
        rv.setTexture("uNormalMap", normal);
        rv.setTexture("uMetallicMap", metallic);
        rv.setTexture("uRoughnessMap", roughness);
        rv.setTexture("uAOMap", ao);
        rv.setVec3Array("uLightWorldPositions", c_LightPositions);
        rv.setVec3Array("uLightRadiances", c_LightRadiances);
        return rv;
    }
}

class osc::LOGLPBRLightingTexturedTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
        m_IsMouseCaptured = false;
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        // handle mouse input
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

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
        updateCameraFromInputs();
        draw3DRender();
        m_PerfPanel.onDraw();
    }

    void updateCameraFromInputs()
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
    }

    void draw3DRender()
    {
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        m_PBRMaterial.setVec3("uCameraWorldPosition", m_Camera.getPosition());

        drawSpheres();
        drawLights();

        m_Camera.renderToScreen();
    }

    void drawSpheres()
    {
        for (int row = 0; row < c_NumRows; ++row)
        {
            for (int col = 0; col < c_NumCols; ++col)
            {
                Transform t;
                t.position =
                {
                    (col - (c_NumCols / 2)) * c_CellSpacing,
                    (row - (c_NumRows / 2)) * c_CellSpacing,
                    0.0f
                };

                Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        for (glm::vec3 const& pos : c_LightPositions)
        {
            Transform t;
            t.position = pos;
            t.scale = glm::vec3{0.5f};

            Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
        }
    }

    Camera m_Camera = CreateCamera();
    Mesh m_SphereMesh = GenSphere(64, 64);
    Material m_PBRMaterial = CreateMaterial();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = true;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

osc::CStringView osc::LOGLPBRLightingTexturedTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab& osc::LOGLPBRLightingTexturedTab::operator=(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab::~LOGLPBRLightingTexturedTab() noexcept = default;

osc::UID osc::LOGLPBRLightingTexturedTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLPBRLightingTexturedTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRLightingTexturedTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRLightingTexturedTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRLightingTexturedTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRLightingTexturedTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPBRLightingTexturedTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPBRLightingTexturedTab::implOnDraw()
{
    m_Impl->onDraw();
}
