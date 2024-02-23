#include "LOGLLightingMapsTab.h"

#include <oscar_learnopengl/LearnOpenGLHelpers.h>
#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/LightingMaps";

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Material CreateLightMappingMaterial(IResourceLoader& rl)
    {
        Texture2D diffuseMap = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/container2.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Texture2D specularMap = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/container2_specular.png"),
            ColorSpace::sRGB,
            ImageLoadingFlags::FlipVertically
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.vert"),
            rl.slurp("oscar_learnopengl/shaders/Lighting/LightingMaps.frag"),
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
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();

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
        ImGui::InputFloat3("uLightPos", value_ptr(m_LightTransform.position));
        ImGui::InputFloat("uLightAmbient", &m_LightAmbient);
        ImGui::InputFloat("uLightDiffuse", &m_LightDiffuse);
        ImGui::InputFloat("uLightSpecular", &m_LightSpecular);
        ImGui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ImGui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_LightingMapsMaterial = CreateLightMappingMaterial(m_Loader);
    Material m_LightCubeMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};
    Mesh m_Mesh = GenerateLearnOpenGLCubeMesh();
    MouseCapturingCamera m_Camera = CreateCamera();

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

UID osc::LOGLLightingMapsTab::implGetID() const
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
