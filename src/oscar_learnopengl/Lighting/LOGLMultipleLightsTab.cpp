#include "LOGLMultipleLightsTab.h"

#include <oscar_learnopengl/LearnOpenGLHelpers.h>
#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/MultipleLights";

    // positions of cubes within the scene
    constexpr auto c_CubePositions = std::to_array<Vec3>({
        { 0.0f,  0.0f,  0.0f },
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f },
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f },
        {-1.7f,  3.0f, -7.5f },
        { 1.3f, -2.0f, -2.5f },
        { 1.5f,  2.0f, -2.5f },
        { 1.5f,  0.2f, -1.5f },
        {-1.3f,  1.0f, -1.5  },
    });

    // positions of point lights within the scene (the camera also has a spotlight)
    constexpr auto c_PointLightPositions = std::to_array<Vec3>({
        { 0.7f,  0.2f,  2.0f },
        { 2.3f, -3.3f, -4.0f },
        {-4.0f,  2.0f, -12.0f},
        { 0.0f,  0.0f, -3.0f },
    });
    constexpr auto c_PointLightAmbients = std::to_array<float>({0.001f, 0.001f, 0.001f, 0.001f});
    constexpr auto c_PointLightDiffuses = std::to_array<float>({0.2f, 0.2f, 0.2f, 0.2f});
    constexpr auto c_PointLightSpeculars = std::to_array<float>({0.5f, 0.5f, 0.5f, 0.5f});
    constexpr auto c_PointLightConstants = std::to_array<float>({1.0f, 1.0f, 1.0f, 1.0f});
    constexpr auto c_PointLightLinears = std::to_array<float>({0.09f, 0.09f, 0.09f, 0.09f});
    constexpr auto c_PointLightQuadratics = std::to_array<float>({0.032f, 0.032f, 0.032f, 0.032f});

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateMultipleLightsMaterial(
        IResourceLoader& rl)
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
            rl.slurp("oscar_learnopengl/shaders/Lighting/MultipleLights.vert"),
            rl.slurp("oscar_learnopengl/shaders/Lighting/MultipleLights.frag"),
        }};

        rv.setTexture("uMaterialDiffuse", diffuseMap);
        rv.setTexture("uMaterialSpecular", specularMap);
        rv.setVec3("uDirLightDirection", {-0.2f, -1.0f, -0.3f});
        rv.setFloat("uDirLightAmbient", 0.01f);
        rv.setFloat("uDirLightDiffuse", 0.2f);
        rv.setFloat("uDirLightSpecular", 0.4f);

        rv.setFloat("uSpotLightAmbient", 0.0f);
        rv.setFloat("uSpotLightDiffuse", 1.0f);
        rv.setFloat("uSpotLightSpecular", 0.75f);

        rv.setFloat("uSpotLightConstant", 1.0f);
        rv.setFloat("uSpotLightLinear", 0.09f);
        rv.setFloat("uSpotLightQuadratic", 0.032f);
        rv.setFloat("uSpotLightCutoff", cos(45_deg));
        rv.setFloat("uSpotLightOuterCutoff", cos(15_deg));

        rv.setVec3Array("uPointLightPos", c_PointLightPositions);
        rv.setFloatArray("uPointLightConstant", c_PointLightConstants);
        rv.setFloatArray("uPointLightLinear", c_PointLightLinears);
        rv.setFloatArray("uPointLightQuadratic", c_PointLightQuadratics);
        rv.setFloatArray("uPointLightAmbient", c_PointLightAmbients);
        rv.setFloatArray("uPointLightDiffuse", c_PointLightDiffuses);
        rv.setFloatArray("uPointLightSpecular", c_PointLightSpeculars);

        return rv;
    }

    Material CreateLightCubeMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/LightCube.vert"),
            rl.slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
        rv.setColor("uLightColor", Color::white());
        return rv;
    }
}

class osc::LOGLMultipleLightsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_LogViewer.open();
        m_PerfPanel.open();
    }

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

        // setup per-frame material vals
        m_MultipleLightsMaterial.setVec3("uViewPos", m_Camera.getPosition());
        m_MultipleLightsMaterial.setFloat("uMaterialShininess", m_MaterialShininess);
        m_MultipleLightsMaterial.setVec3("uSpotLightPosition", m_Camera.getPosition());
        m_MultipleLightsMaterial.setVec3("uSpotLightDirection", m_Camera.getDirection());

        // render containers
        UnitVec3 const axis{1.0f, 0.3f, 0.5f};
        for (size_t i = 0; i < c_CubePositions.size(); ++i) {
            Vec3 const& pos = c_CubePositions[i];
            auto const angle = i++ * 20_deg;

            Graphics::DrawMesh(
                m_Mesh,
                {.rotation = angle_axis(angle, axis), .position = pos},
                m_MultipleLightsMaterial,
                m_Camera
            );
        }

        // render lamps
        for (Vec3 const& pos : c_PointLightPositions) {
            Graphics::DrawMesh(m_Mesh, {.scale = Vec3{0.2f}, .position = pos}, m_LightCubeMaterial, m_Camera);
        }

        // render to output (window)
        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        // render auxiliary UI
        ui::Begin("controls");
        ui::InputFloat("uMaterialShininess", &m_MaterialShininess);
        ui::End();

        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    ResourceLoader m_Loader = App::resource_loader();

    Material m_MultipleLightsMaterial = CreateMultipleLightsMaterial(m_Loader);
    Material m_LightCubeMaterial = CreateLightCubeMaterial(m_Loader);
    Mesh m_Mesh = GenerateLearnOpenGLCubeMesh();

    MouseCapturingCamera m_Camera = CreateCamera();

    float m_MaterialShininess = 64.0f;

    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

CStringView osc::LOGLMultipleLightsTab::id()
{
    return c_TabStringID;
}

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLMultipleLightsTab::LOGLMultipleLightsTab(LOGLMultipleLightsTab&&) noexcept = default;
osc::LOGLMultipleLightsTab& osc::LOGLMultipleLightsTab::operator=(LOGLMultipleLightsTab&&) noexcept = default;
osc::LOGLMultipleLightsTab::~LOGLMultipleLightsTab() noexcept = default;

UID osc::LOGLMultipleLightsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLMultipleLightsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLMultipleLightsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLMultipleLightsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLMultipleLightsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLMultipleLightsTab::implOnDraw()
{
    m_Impl->onDraw();
}
