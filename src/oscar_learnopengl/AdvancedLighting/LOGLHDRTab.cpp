#include "LOGLHDRTab.hpp"

#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <array>
#include <numbers>
#include <string>
#include <utility>

using osc::Vec3;

namespace
{
    constexpr auto c_LightPositions = std::to_array<Vec3>(
    {
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    });

    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/HDR";

    std::array<osc::Color, c_LightPositions.size()> GetLightColors()
    {
        return std::to_array<osc::Color>(
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

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    osc::Material CreateSceneMaterial()
    {
        osc::Texture2D woodTexture = osc::LoadTexture2DFromImage(
            osc::App::resource("oscar_learnopengl/textures/wood.png"),
            osc::ColorSpace::sRGB
        );

        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.vert"),
                osc::App::slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Scene.frag"),
            },
        };
        rv.setVec3Array("uSceneLightPositions", c_LightPositions);
        rv.setColorArray("uSceneLightColors", GetLightColors());
        rv.setTexture("uDiffuseTexture", woodTexture);
        rv.setBool("uInverseNormals", true);
        return rv;
    }

    osc::Material CreateTonemapMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.vert"),
                osc::App::slurp("oscar_learnopengl/shaders/AdvancedLighting/HDR/Tonemap.frag"),
            },
        };
    }
}

class osc::LOGLHDRTab::Impl final : public osc::StandardTabBase {
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
            descriptor.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());
            if (m_Use16BitFormat)
            {
                descriptor.setColorFormat(RenderTextureFormat::ARGBFloat16);
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
        orthoCamera.setProjectionMatrixOverride(Mat4{1.0f});
        orthoCamera.setViewMatrixOverride(Mat4{1.0f});

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

    Material m_SceneMaterial = CreateSceneMaterial();
    Material m_TonemapMaterial = CreateTonemapMaterial();
    Camera m_Camera = CreateSceneCamera();
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();
    Transform m_CorridoorTransform = CalcCorridoorTransform();
    RenderTexture m_SceneHDRTexture;
    float m_Exposure = 1.0f;

    Vec3 m_CameraEulers = {0.0f, std::numbers::pi_v<float>, 0.0f};
    bool m_IsMouseCaptured = true;
    bool m_Use16BitFormat = true;
    bool m_UseTonemap = true;
};


// public API

osc::CStringView osc::LOGLHDRTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLHDRTab::LOGLHDRTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
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
