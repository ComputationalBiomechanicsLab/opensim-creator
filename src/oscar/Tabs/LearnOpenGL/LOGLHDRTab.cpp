#include "LOGLHDRTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/RenderTextureDescriptor.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec3.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <array>
#include <string>
#include <utility>

namespace
{
    auto constexpr c_LightPositions = osc::to_array<glm::vec3>(
    {
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    });

    osc::CStringView constexpr c_TabStringID = "LearnOpenGL/HDR";

    std::array<osc::Color, c_LightPositions.size()> GetLightColors()
    {
        return osc::to_array<osc::Color>(
        {
            osc::ToSRGB({200.0f, 200.0f, 200.0f, 1.0f}),
            osc::ToSRGB({0.1f, 0.0f, 0.0f, 1.0f}),
            osc::ToSRGB({0.0f, 0.0f, 0.2f, 1.0f}),
            osc::ToSRGB({0.0f, 0.1f, 0.0f, 1.0f}),
        });
    }

    osc::Transform CalcCorridoorTransform()
    {
        osc::Transform rv;
        rv.position = {0.0f, 0.0f, 25.0f};
        rv.scale = {2.5f, 2.5f, 27.5f};
        return rv;
    }
}

class osc::LOGLHDRTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_SceneMaterial.setVec3Array("uSceneLightPositions", c_LightPositions);
        m_SceneMaterial.setColorArray("uSceneLightColors", GetLightColors());
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
        orthoCamera.setBackgroundColor(Color::clear());
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
    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        App::resource("textures/wood.png"),
        ColorSpace::sRGB
    );
    Transform m_CorridoorTransform = CalcCorridoorTransform();
    RenderTexture m_SceneHDRTexture;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, osc::fpi, 0.0f};
    bool m_Use16BitFormat = true;
    bool m_UseTonemap = true;
    float m_Exposure = 1.0f;
};


// public API (PIMPL)

osc::CStringView osc::LOGLHDRTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLHDRTab::LOGLHDRTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::LOGLHDRTab::LOGLHDRTab(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab& osc::LOGLHDRTab::operator=(LOGLHDRTab&&) noexcept = default;
osc::LOGLHDRTab::~LOGLHDRTab() noexcept = default;

osc::UID osc::LOGLHDRTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLHDRTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHDRTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLHDRTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLHDRTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLHDRTab::implOnDraw()
{
    m_Impl->onDraw();
}
