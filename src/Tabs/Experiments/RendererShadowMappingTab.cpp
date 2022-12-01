#include "RendererShadowMappingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    // this matches the plane vertices used in the LearnOpenGL tutorial
    osc::Mesh GeneratePlaneMesh()
    {
        std::vector<glm::vec3> const verts =
        {
            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},

            { 25.0f, -0.5f,  25.0f},
            {-25.0f, -0.5f, -25.0f},
            { 25.0f, -0.5f, -25.0f},
        };

        std::vector<glm::vec3> const normals =
        {
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},

            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        };

        std::vector<glm::vec2> const texCoords =
        {
            {25.0f,  0.0f},
            {0.0f,  0.0f},
            {0.0f, 25.0f},

            {25.0f,  0.0f},
            {0.0f, 25.0f},
            {25.0f, 25.0f},
        };

        std::vector<uint16_t> const indices = {0, 1, 2, 3, 4, 5};

        osc::Mesh rv;
        rv.setVerts(verts);
        rv.setNormals(normals);
        rv.setTexCoords(texCoords);
        rv.setIndices(indices);
        return rv;
    }
}

class osc::RendererShadowMappingTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
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
        App::upd().makeMainEventLoopWaiting();
        App::upd().setShowCursor(true);
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
        handleMouseCapture();
        drawScene();
    }

private:
    void handleMouseCapture()
    {
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

    void drawScene()
    {
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});

        m_SceneMaterial.setVec3("uLightColor", glm::vec3{0.3f});
        m_SceneMaterial.setVec3("uLightWorldPos", {-2.0f, 4.0f, -1.0f});
        m_SceneMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
        m_SceneMaterial.setTexture("uDiffuseTexture", m_WoodTexture);

        // floor
        Graphics::DrawMesh(m_PlaneMesh, Transform{}, m_SceneMaterial, m_Camera);

        // cubes
        {
            Transform t;
            t.position = {0.0f, 1.0f, 0.0f};
            t.scale = glm::vec3{0.5f};
            Graphics::DrawMesh(m_CubeMesh, t, m_SceneMaterial, m_Camera);
        }
        {
            Transform t;
            t.position = {2.0f, 0.0f, 1.0f};
            t.scale = glm::vec3{0.5f};
            Graphics::DrawMesh(m_CubeMesh, t, m_SceneMaterial, m_Camera);
        }
        {
            Transform t;
            t.position = {-1.0f, 0.0f, 2.0f};
            t.rotation = glm::angleAxis(glm::radians(60.0f), glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f}));
            t.scale = glm::vec3{0.25f};
            Graphics::DrawMesh(m_CubeMesh, t, m_SceneMaterial, m_Camera);
        }

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    // tab state
    UID m_ID;
    std::string m_Name = ICON_FA_BOOK " ShadowMapping";
    TabHost* m_Parent;
    bool m_IsMouseCaptured = false;

    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Texture2D m_WoodTexture = osc::LoadTexture2DFromImage(App::resource("textures/wood.png"));
    Mesh m_CubeMesh = osc::GenCube();
    Mesh m_PlaneMesh = GeneratePlaneMesh();
    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentShadowMapping.vert"),
            App::slurp("shaders/ExperimentShadowMapping.frag"),
        }
    };
    glm::vec3 m_LightPos = {-2.0f, 4.0f, -1.0f};
};


// public API (PIMPL)

osc::RendererShadowMappingTab::RendererShadowMappingTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::RendererShadowMappingTab::RendererShadowMappingTab(RendererShadowMappingTab&&) noexcept = default;
osc::RendererShadowMappingTab& osc::RendererShadowMappingTab::operator=(RendererShadowMappingTab&&) noexcept = default;
osc::RendererShadowMappingTab::~RendererShadowMappingTab() noexcept = default;

osc::UID osc::RendererShadowMappingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererShadowMappingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererShadowMappingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::RendererShadowMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererShadowMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererShadowMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererShadowMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererShadowMappingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererShadowMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
