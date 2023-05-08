#include "LOGLGammaTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/ColorSpace.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <string>
#include <utility>

static glm::vec3 constexpr c_PlaneVertices[] =
{
    { 10.0f, -0.5f,  10.0f},
    {-10.0f, -0.5f,  10.0f},
    {-10.0f, -0.5f, -10.0f},

    { 10.0f, -0.5f,  10.0f},
    {-10.0f, -0.5f, -10.0f},
    { 10.0f, -0.5f, -10.0f},
};

static glm::vec2 constexpr c_PlaneTexCoords[] =
{
    {10.0f, 0.0f},
    {0.0f,  0.0f},
    {0.0f,  10.0f},

    {10.0f, 0.0f},
    {0.0f,  10.0f},
    {10.0f, 10.0f},
};

static glm::vec3 constexpr c_PlaneNormals[] =
{
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},

    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
};

static uint16_t constexpr c_PlaneIndices[] = {0, 2, 1, 3, 5, 4};

static glm::vec3 constexpr c_LightPositions[] =
{
    {-3.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { 3.0f, 0.0f, 0.0f},
};

static osc::Color constexpr c_LightColors[] =
{
    {0.25f, 0.25f, 0.25f, 1.0f},
    {0.50f, 0.50f, 0.50f, 1.0f},
    {0.75f, 0.75f, 0.75f, 1.0f},
    {1.00f, 1.00f, 1.00f, 1.0f},
};

constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Gamma";

namespace
{
    osc::Mesh GeneratePlane()
    {
        osc::Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setNormals(c_PlaneNormals);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_Material.setTexture("uFloorTexture", m_WoodTexture);
        m_Material.setVec3Array("uLightPositions", c_LightPositions);
        m_Material.setColorArray("uLightColors", c_LightColors);

        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
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

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/ExperimentGamma.vert"),
            App::slurp("shaders/ExperimentGamma.frag"),
        }
    };
    Mesh m_PlaneMesh = GeneratePlane();
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        App::resource("textures/wood.png"),
        ColorSpace::sRGB
    );

    Camera m_Camera;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
};


// public API (PIMPL)

osc::CStringView osc::LOGLGammaTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLGammaTab::LOGLGammaTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
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
