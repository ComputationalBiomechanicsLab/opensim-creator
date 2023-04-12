#include "RendererHDRTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <array>
#include <string>
#include <utility>

static glm::vec3 constexpr c_LightPositions[] =
{
    { 0.0f,  0.0f, 49.5f},
    {-1.4f, -1.9f, 9.0f},
    { 0.0f, -1.8f, 4.0f},
    { 0.8f, -1.7f, 6.0f},
};

static glm::vec3 constexpr c_LightColors[] =
{
    {200.0f, 200.0f, 200.0f},
    {0.1f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.2f},
    {0.0f, 0.1f, 0.0f},
};
static_assert(sizeof(c_LightPositions) == sizeof(c_LightColors));

namespace
{
    osc::Transform CalcCorridoorTransform()
    {
        osc::Transform rv;
        rv.position = {0.0f, 0.0f, 25.0f};
        rv.scale = {2.5f, 2.5f, 27.5f};
        return rv;
    }
}

class osc::RendererHDRTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_SceneMaterial.setVec3Array("uSceneLightPositions", c_LightPositions);
        m_SceneMaterial.setVec3Array("uSceneLightColors", c_LightColors);
        m_SceneMaterial.setTexture("uDiffuseTexture", m_WoodTexture);
        m_SceneMaterial.setBool("uInverseNormals", true);

        m_Camera.setPosition({0.0f, 0.0f, 5.0f});
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
        return ICON_FA_COOKIE " RendererHDRTab";
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
        handleMouseCapturing();
        draw3DSceneToHDRTexture();
        drawHDRTextureViaTonemapperToScreen();
        draw2DUI();
    }

    void handleMouseCapturing()
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

    void draw3DSceneToHDRTexture()
    {
        // reformat intermediate HDR texture to match tab dimensions etc.
        {
            Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
            RenderTextureDescriptor descriptor{Dimensions(viewportRect)};
            descriptor.setAntialiasingLevel(App::get().getMSXAASamplesRecommended());
            if (m_Use16BitFormat)
            {
                descriptor.setColorFormat(RenderTextureFormat::ARGBHalf);
            }

            m_SceneHDRTexture.reformat(descriptor);
        }

        Graphics::DrawMesh(m_CubeMesh, m_CorridoorTransform, m_SceneMaterial, m_Camera);
        m_Camera.renderTo(m_SceneHDRTexture);
    }

    void drawHDRTextureViaTonemapperToScreen()
    {
        Camera orthoCamera;
        orthoCamera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
        orthoCamera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        orthoCamera.setProjectionMatrixOverride(glm::mat4{1.0f});
        orthoCamera.setViewMatrixOverride(glm::mat4{1.0f});

        m_TonemapMaterial.setRenderTexture("uTexture", m_SceneHDRTexture);
        m_TonemapMaterial.setBool("uUseTonemap", m_UseTonemap);
        m_TonemapMaterial.setFloat("uExposure", m_Exposure);

        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_TonemapMaterial, orthoCamera);
        orthoCamera.renderToScreen();

        m_TonemapMaterial.clearRenderTexture("uTexture");
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        ImGui::Checkbox("use tonemapping", &m_UseTonemap);
        ImGui::Checkbox("use 16-bit colors", &m_Use16BitFormat);
        ImGui::InputFloat("exposure", &m_Exposure);
        ImGui::Text("pos = %f,%f,%f", m_Camera.getPosition().x, m_Camera.getPosition().y, m_Camera.getPosition().z);
        ImGui::Text("eulers = %f,%f,%f", m_CameraEulers.x, m_CameraEulers.y, m_CameraEulers.z);
        ImGui::End();
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentHDRScene.vert"),
            App::slurp("shaders/ExperimentHDRScene.frag"),
        },
    };
    Material m_TonemapMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentHDRTonemap.vert"),
            App::slurp("shaders/ExperimentHDRTonemap.frag"),
        },
    };
    Camera m_Camera;
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();
    Texture2D m_WoodTexture = LoadTexture2DFromImage(App::resource("textures/wood.png"));  // TODO: provide gamma-corrected load function
    Transform m_CorridoorTransform = CalcCorridoorTransform();
    RenderTexture m_SceneHDRTexture;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, osc::fpi, 0.0f};
    bool m_Use16BitFormat = true;
    bool m_UseTonemap = true;
    float m_Exposure = 1.0f;
};


// public API (PIMPL)

osc::CStringView osc::RendererHDRTab::id() noexcept
{
    return "Renderer/HDR";
}

osc::RendererHDRTab::RendererHDRTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererHDRTab::RendererHDRTab(RendererHDRTab&&) noexcept = default;
osc::RendererHDRTab& osc::RendererHDRTab::operator=(RendererHDRTab&&) noexcept = default;
osc::RendererHDRTab::~RendererHDRTab() noexcept = default;

osc::UID osc::RendererHDRTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererHDRTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererHDRTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererHDRTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererHDRTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererHDRTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererHDRTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererHDRTab::implOnDraw()
{
    m_Impl->onDraw();
}
