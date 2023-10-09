#include "LOGLLightingMapsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/LightingMaps";

    osc::Camera CreateCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    osc::Material CreateLightMappingMaterial()
    {
        osc::Texture2D diffuseMap = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/container2.png"),
            osc::ColorSpace::sRGB,
            osc::ImageLoadingFlags::FlipVertically
        );
        osc::Texture2D specularMap = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/container2_specular.png"),
            osc::ColorSpace::sRGB,
            osc::ImageLoadingFlags::FlipVertically
        );

        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/Lighting/LightingMaps.vert"),
                osc::App::slurp("shaders/LearnOpenGL/Lighting/LightingMaps.frag"),
            },
        };
        rv.setTexture("uMaterialDiffuse", diffuseMap);
        rv.setTexture("uMaterialSpecular", specularMap);
        return rv;
    }
}

class osc::LOGLLightingMapsTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        m_LightTransform.position = {0.4f, 0.4f, 2.0f};
        m_LightTransform.scale = {0.2f, 0.2f, 0.2f};
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

    Material m_LightingMapsMaterial = CreateLightMappingMaterial();
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/LearnOpenGL/LightCube.vert"),
            App::slurp("shaders/LearnOpenGL/LightCube.frag"),
        },
    };
    Mesh m_Mesh = GenLearnOpenGLCube();
    Camera m_Camera = CreateCamera();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = false;

    Transform m_LightTransform;
    float m_LightAmbient = 0.02f;
    float m_LightDiffuse = 0.4f;
    float m_LightSpecular = 1.0f;
    float m_MaterialShininess = 64.0f;
};


// public API

osc::CStringView osc::LOGLLightingMapsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLLightingMapsTab::LOGLLightingMapsTab(ParentPtr<TabHost> const&) :
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
