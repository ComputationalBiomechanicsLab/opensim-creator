#include "LOGLNormalMappingTab.h"

#include <oscar_learnopengl/LearnOpenGLHelpers.h>
#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/NormalMapping";

    // matches the quad used in LearnOpenGL's normal mapping tutorial
    Mesh GenerateQuad()
    {
        Mesh rv;
        rv.setVerts({
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        rv.setNormals({
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        rv.setTexCoords({
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        rv.setIndices({
            0, 1, 2,
            0, 2, 3,
        });
        rv.recalculateTangents();
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Material CreateNormalMappingMaterial(IResourceLoader& rl)
    {
        Texture2D diffuseMap = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/brickwall.jpg"),
            ColorSpace::sRGB
        );
        Texture2D normalMap = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/brickwall_normal.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.frag"),
        }};
        rv.setTexture("uDiffuseMap", diffuseMap);
        rv.setTexture("uNormalMap", normalMap);

        return rv;
    }

    Material CreateLightCubeMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/LightCube.vert"),
            rl.slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLNormalMappingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnTick() final
    {
        // rotate the quad over time
        AppClock::duration const dt = App::get().getFrameDeltaSinceAppStartup();
        auto const angle = Degrees{-10.0 * dt.count()};
        auto const axis = UnitVec3{1.0f, 0.0f, 1.0f};
        m_QuadTransform.rotation = angle_axis(angle, axis);
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw normal-mapped quad
        {
            m_NormalMappingMaterial.setVec3("uLightWorldPos", m_LightTransform.position);
            m_NormalMappingMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
            m_NormalMappingMaterial.setBool("uEnableNormalMapping", m_IsNormalMappingEnabled);
            Graphics::DrawMesh(m_QuadMesh, m_QuadTransform, m_NormalMappingMaterial, m_Camera);
        }

        // draw light source cube
        {
            m_LightCubeMaterial.setColor("uLightColor", Color::white());
            Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);
        }

        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        ui::Begin("controls");
        ui::Checkbox("normal mapping", &m_IsNormalMappingEnabled);
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();

    // rendering state
    Material m_NormalMappingMaterial = CreateNormalMappingMaterial(m_Loader);
    Material m_LightCubeMaterial = CreateLightCubeMaterial(m_Loader);
    Mesh m_CubeMesh = GenerateLearnOpenGLCubeMesh();
    Mesh m_QuadMesh = GenerateQuad();

    // scene state
    MouseCapturingCamera m_Camera = CreateCamera();
    Transform m_QuadTransform;
    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool m_IsNormalMappingEnabled = true;
};


// public API

CStringView osc::LOGLNormalMappingTab::id()
{
    return c_TabStringID;
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab& osc::LOGLNormalMappingTab::operator=(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab::~LOGLNormalMappingTab() noexcept = default;

UID osc::LOGLNormalMappingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLNormalMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLNormalMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLNormalMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLNormalMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLNormalMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLNormalMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
