#include "LOGLLightingMapsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using osc::App;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::ImageLoadingFlags;
using osc::LoadTexture2DFromImage;
using osc::Material;
using osc::Shader;
using osc::Texture2D;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/LightingMaps";

    Camera CreateCamera()
    {
        Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Material CreateLightMappingMaterial()
    {
        Texture2D diffuseMap = LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/container2.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Texture2D specularMap = LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/container2_specular.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.vert"),
            App::slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.frag"),
        }};
        rv.setTexture("uMaterialDiffuse", diffuseMap);
        rv.setTexture("uMaterialSpecular", specularMap);
        return rv;
    }
}

class osc::LOGLLightingMapsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

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
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect()) {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void implOnDraw() final
    {
        // handle mouse capturing
        if (m_IsMouseCaptured) {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw cube
        m_LightingMapsMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMapsMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMapsMaterial.setFloat("uLightAmbient", m_LightAmbient);
        m_LightingMapsMaterial.setFloat("uLightDiffuse", m_LightDiffuse);
        m_LightingMapsMaterial.setFloat("uLightSpecular", m_LightSpecular);
        m_LightingMapsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        Graphics::DrawMesh(m_Mesh, Identity<Transform>(), m_LightingMapsMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setColor("uLightColor", Color::white());
        Graphics::DrawMesh(m_Mesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render 3D scene
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        // render 2D UI
        ImGui::Begin("controls");
        ImGui::InputFloat3("uLightPos", ValuePtr(m_LightTransform.position));
        ImGui::InputFloat("uLightAmbient", &m_LightAmbient);
        ImGui::InputFloat("uLightDiffuse", &m_LightDiffuse);
        ImGui::InputFloat("uLightSpecular", &m_LightSpecular);
        ImGui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ImGui::End();
    }

    Material m_LightingMapsMaterial = CreateLightMappingMaterial();
    Material m_LightCubeMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/LightCube.vert"),
        App::slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};
    Mesh m_Mesh = GenerateLearnOpenGLCubeMesh();
    Camera m_Camera = CreateCamera();
    Eulers m_CameraEulers = {};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.4f, 0.4f, 2.0f},
    };
    float m_LightAmbient = 0.02f;
    float m_LightDiffuse = 0.4f;
    float m_LightSpecular = 1.0f;
    float m_MaterialShininess = 64.0f;
};


// public API

CStringView osc::LOGLLightingMapsTab::id()
{
    return c_TabStringID;
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab& osc::LOGLLightingMapsTab::operator=(LOGLLightingMapsTab&&) noexcept = default;
osc::LOGLLightingMapsTab::~LOGLLightingMapsTab() noexcept = default;

osc::UID osc::LOGLLightingMapsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLLightingMapsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLLightingMapsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLLightingMapsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLLightingMapsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLLightingMapsTab::implOnDraw()
{
    m_Impl->onDraw();
}
