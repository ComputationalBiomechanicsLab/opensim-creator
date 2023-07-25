#include "LOGLLightingMapsTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/LightingMaps";
}

class osc::LOGLLightingMapsTab::Impl final {
public:

    Impl()
    {
        m_LightingMapsMaterial.setTexture("uMaterialDiffuse", m_DiffuseMap);
        m_LightingMapsMaterial.setTexture("uMaterialSpecular", m_SpecularMap);
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_LightTransform.position = {0.4f, 0.4f, 2.0f};
        m_LightTransform.scale = {0.2f, 0.2f, 0.2f};
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

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw cube
        m_LightingMapsMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMapsMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMapsMaterial.setFloat("uLightAmbient", m_LightAmbient);
        m_LightingMapsMaterial.setFloat("uLightDiffuse", m_LightDiffuse);
        m_LightingMapsMaterial.setFloat("uLightSpecular", m_LightSpecular);
        m_LightingMapsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        Graphics::DrawMesh(m_Mesh, Transform{}, m_LightingMapsMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setColor("uLightColor", Color::white());
        osc::Graphics::DrawMesh(m_Mesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render 3D scene
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        // render 2D UI
        ImGui::Begin("controls");
        ImGui::InputFloat3("uLightPos", glm::value_ptr(m_LightTransform.position));
        ImGui::InputFloat("uLightAmbient", &m_LightAmbient);
        ImGui::InputFloat("uLightDiffuse", &m_LightDiffuse);
        ImGui::InputFloat("uLightSpecular", &m_LightSpecular);
        ImGui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ImGui::End();
    }

private:
    UID m_TabID;

    Material m_LightingMapsMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightingMaps.vert"),
            App::slurp("shaders/ExperimentLightingMaps.frag"),
        },
    };
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightCube.vert"),
            App::slurp("shaders/ExperimentLightCube.frag"),
        },
    };
    Mesh m_Mesh = GenLearnOpenGLCube();
    Texture2D m_DiffuseMap = LoadTexture2DFromImage(
        App::resource("textures/container2.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );
    Texture2D m_SpecularMap = LoadTexture2DFromImage(
        App::resource("textures/container2_specular.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );

    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform;
    float m_LightAmbient = 0.02f;
    float m_LightDiffuse = 0.4f;
    float m_LightSpecular = 1.0f;
    float m_MaterialShininess = 64.0f;
};


// public API (PIMPL)

osc::CStringView osc::LOGLLightingMapsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(std::weak_ptr<TabHost>) :
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

osc::CStringView osc::LOGLLightingMapsTab::implGetName() const
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
