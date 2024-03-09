#include "LOGLBasicLightingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/BasicLighting";

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLBasicLightingTab::Impl final : public StandardTabImpl {
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
        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());

        // draw cube
        m_LightingMaterial.setColor("uObjectColor", m_ObjectColor);
        m_LightingMaterial.setColor("uLightColor", m_LightColor);
        m_LightingMaterial.setVec3("uLightPos", m_LightTransform.position);
        m_LightingMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_LightingMaterial.setFloat("uAmbientStrength", m_AmbientStrength);
        m_LightingMaterial.setFloat("uDiffuseStrength", m_DiffuseStrength);
        m_LightingMaterial.setFloat("uSpecularStrength", m_SpecularStrength);
        Graphics::DrawMesh(m_CubeMesh, identity<Transform>(), m_LightingMaterial, m_Camera);

        // draw lamp
        m_LightCubeMaterial.setColor("uLightColor", m_LightColor);
        Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);

        // render to output (window)
        m_Camera.renderToScreen();

        // render auxiliary UI
        ui::Begin("controls");
        ui::InputVec3("light pos", m_LightTransform.position);
        ui::InputFloat("ambient strength", &m_AmbientStrength);
        ui::InputFloat("diffuse strength", &m_DiffuseStrength);
        ui::InputFloat("specular strength", &m_SpecularStrength);
        ui::ColorEditRGB("object color", m_ObjectColor);
        ui::ColorEditRGB("light color", m_LightColor);
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();

    Material m_LightingMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/Lighting/BasicLighting.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/Lighting/BasicLighting.frag"),
    }};

    Material m_LightCubeMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/LightCube.frag"),
    }};

    Mesh m_CubeMesh = BoxGeometry{};

    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();

    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {1.2f, 1.0f, 2.0f},
    };
    Color m_ObjectColor = {1.0f, 0.5f, 0.31f, 1.0f};
    Color m_LightColor = Color::white();
    float m_AmbientStrength = 0.01f;
    float m_DiffuseStrength = 0.6f;
    float m_SpecularStrength = 1.0f;
};


// public API

CStringView osc::LOGLBasicLightingTab::id()
{
    return c_TabStringID;
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBasicLightingTab::LOGLBasicLightingTab(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab& osc::LOGLBasicLightingTab::operator=(LOGLBasicLightingTab&&) noexcept = default;
osc::LOGLBasicLightingTab::~LOGLBasicLightingTab() noexcept = default;

UID osc::LOGLBasicLightingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLBasicLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBasicLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBasicLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBasicLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBasicLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
