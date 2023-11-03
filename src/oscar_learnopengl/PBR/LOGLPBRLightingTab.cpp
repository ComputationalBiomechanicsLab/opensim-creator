#include "LOGLPBRLightingTab.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <string>
#include <utility>

using osc::Vec3;

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/Lighting";

    constexpr auto c_LightPositions = osc::to_array<Vec3>(
    {
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_LightPositions.size()> c_LightRadiances = osc::to_array<Vec3>(
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
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    osc::Material CreateMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.vert"),
                osc::App::slurp("oscar_learnopengl/shaders/PBR/lighting/PBR.frag"),
            },
        };
        rv.setFloat("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRLightingTab::Impl final : public osc::StandardTabBase {
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
        draw2DUI();
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

        m_PBRMaterial.setVec3("uCameraWorldPos", m_Camera.getPosition());
        m_PBRMaterial.setVec3Array("uLightPositions", c_LightPositions);
        m_PBRMaterial.setVec3Array("uLightColors", c_LightRadiances);

        drawSpheres();
        drawLights();

        m_Camera.renderToScreen();
    }

    void drawSpheres()
    {
        m_PBRMaterial.setVec3("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_NumRows; ++row)
        {
            m_PBRMaterial.setFloat("uMetallicity", static_cast<float>(row) / static_cast<float>(c_NumRows));

            for (int col = 0; col < c_NumCols; ++col)
            {
                float const normalizedCol = static_cast<float>(col) / static_cast<float>(c_NumCols);
                m_PBRMaterial.setFloat("uRoughness", osc::Clamp(normalizedCol, 0.005f, 1.0f));

                Transform t;
                t.position =
                {
                    (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing,
                    (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing,
                    0.0f
                };

                Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        m_PBRMaterial.setVec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (Vec3 const& pos : c_LightPositions)
        {
            Transform t;
            t.position = pos;
            t.scale = Vec3{0.5f};

            Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
        }
    }

    void draw2DUI()
    {
        m_PerfPanel.onDraw();
    }

    Camera m_Camera = CreateCamera();
    Mesh m_SphereMesh = GenSphere(64, 64);
    Material m_PBRMaterial = CreateMaterial();
    Vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = true;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

osc::CStringView osc::LOGLPBRLightingTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab& osc::LOGLPBRLightingTab::operator=(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab::~LOGLPBRLightingTab() noexcept = default;

osc::UID osc::LOGLPBRLightingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLPBRLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRLightingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPBRLightingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPBRLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
