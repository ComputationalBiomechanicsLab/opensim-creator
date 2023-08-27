#include "LOGLGammaTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr auto c_PlaneVertices = osc::to_array<glm::vec3>(
    {
        { 10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f, -10.0f},

        { 10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f, -10.0f},
        { 10.0f, -0.5f, -10.0f},
    });
    constexpr auto c_PlaneTexCoords = osc::to_array<glm::vec2>(
    {
        {10.0f, 0.0f},
        {0.0f,  0.0f},
        {0.0f,  10.0f},

        {10.0f, 0.0f},
        {0.0f,  10.0f},
        {10.0f, 10.0f},
    });
    constexpr auto c_PlaneNormals = osc::to_array<glm::vec3>(
    {
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},

        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    constexpr auto c_PlaneIndices = osc::to_array<uint16_t>({0, 2, 1, 3, 5, 4});

    constexpr auto c_LightPositions = osc::to_array<glm::vec3>(
    {
        {-3.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f},
        { 3.0f, 0.0f, 0.0f},
    });

    constexpr auto c_LightColors = osc::to_array<osc::Color>(
    {
        {0.25f, 0.25f, 0.25f, 1.0f},
        {0.50f, 0.50f, 0.50f, 1.0f},
        {0.75f, 0.75f, 0.75f, 1.0f},
        {1.00f, 1.00f, 1.00f, 1.0f},
    });

    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Gamma";

    osc::Mesh GeneratePlane()
    {
        osc::Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setNormals(c_PlaneNormals);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    osc::Material CreateFloorMaterial()
    {
        osc::Texture2D woodTexture = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/wood.png"),
            osc::ColorSpace::sRGB
        );

        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/Gamma.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/Gamma.frag"),
            },
        };
        rv.setTexture("uFloorTexture", woodTexture);
        rv.setVec3Array("uLightPositions", c_LightPositions);
        rv.setColorArray("uLightColors", c_LightColors);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final : public osc::StandardTabBase {
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

        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // render scene
        m_Material.setVec3("uViewPos", m_Camera.getPosition());
        Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        ImGui::Text("no need to gamma correct - OSC is a gamma-corrected renderer");
        ImGui::End();
    }

    Material m_Material = CreateFloorMaterial();
    Mesh m_PlaneMesh = GeneratePlane();
    Camera m_Camera = CreateSceneCamera();
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {};
};


// public API

osc::CStringView osc::LOGLGammaTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLGammaTab::LOGLGammaTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLGammaTab::LOGLGammaTab(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab& osc::LOGLGammaTab::operator=(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab::~LOGLGammaTab() noexcept = default;

osc::UID osc::LOGLGammaTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLGammaTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLGammaTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLGammaTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLGammaTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLGammaTab::implOnDraw()
{
    m_Impl->onDraw();
}
