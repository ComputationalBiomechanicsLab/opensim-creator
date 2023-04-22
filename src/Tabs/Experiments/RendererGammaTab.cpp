#include "RendererGammaTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
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

class osc::RendererGammaTab::Impl final {
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
        return ICON_FA_COOKIE " RendererGammaTab";
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

        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

        // render scene
        m_Material.setVec3("uViewPos", m_Camera.getPosition());
        m_Material.setBool("uGamma", m_IsGammaCorrected);
        Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        ImGui::Text("TODO: requires sRGB/linear colorspace support in backend (osc::Color, etc.)");
        ImGui::Checkbox("gamma corrected", &m_IsGammaCorrected);
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
    Texture2D m_WoodTexture = LoadTexture2DFromImage(App::resource("textures/wood.png"));  // TODO: provide gamma-corrected load function

    Camera m_Camera;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};

    bool m_IsGammaCorrected = false;
};


// public API (PIMPL)

osc::CStringView osc::RendererGammaTab::id() noexcept
{
    return "Renderer/Gamma";
}

osc::RendererGammaTab::RendererGammaTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererGammaTab::RendererGammaTab(RendererGammaTab&&) noexcept = default;
osc::RendererGammaTab& osc::RendererGammaTab::operator=(RendererGammaTab&&) noexcept = default;
osc::RendererGammaTab::~RendererGammaTab() noexcept = default;

osc::UID osc::RendererGammaTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererGammaTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererGammaTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererGammaTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererGammaTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererGammaTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererGammaTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererGammaTab::implOnDraw()
{
    m_Impl->onDraw();
}
