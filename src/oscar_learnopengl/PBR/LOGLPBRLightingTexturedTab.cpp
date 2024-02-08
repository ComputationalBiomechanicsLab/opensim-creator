#include "LOGLPBRLightingTexturedTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <oscar/oscar.hpp>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/LightingTextured";

    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_LightPositions.size()> c_LightRadiances = std::to_array<Vec3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_NumRows = 7;
    constexpr int c_NumCols = 7;
    constexpr float c_CellSpacing = 2.5f;

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 20.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateMaterial(IResourceLoader& rl)
    {
        Texture2D albedo = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/albedo.png"),
            ColorSpace::sRGB
        );
        Texture2D normal = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/normal.png"),
            ColorSpace::Linear
        );
        Texture2D metallic = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/metallic.png"),
            ColorSpace::Linear
        );
        Texture2D roughness = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/roughness.png"),
            ColorSpace::Linear
        );
        Texture2D ao = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/pbr/rusted_iron/ao.png"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting_textured/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/lighting_textured/PBR.frag"),
        }};
        rv.setTexture("uAlbedoMap", albedo);
        rv.setTexture("uNormalMap", normal);
        rv.setTexture("uMetallicMap", metallic);
        rv.setTexture("uRoughnessMap", roughness);
        rv.setTexture("uAOMap", ao);
        rv.setVec3Array("uLightWorldPositions", c_LightPositions);
        rv.setVec3Array("uLightRadiances", c_LightRadiances);
        return rv;
    }
}

class osc::LOGLPBRLightingTexturedTab::Impl final : public StandardTabImpl {
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
        draw3DRender();
        m_PerfPanel.onDraw();
    }

    void draw3DRender()
    {
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        m_PBRMaterial.setVec3("uCameraWorldPosition", m_Camera.getPosition());

        drawSpheres();
        drawLights();

        m_Camera.renderToScreen();
    }

    void drawSpheres()
    {
        for (int row = 0; row < c_NumRows; ++row) {
            for (int col = 0; col < c_NumCols; ++col) {
                float const x = (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing;
                float const y = (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing;
                Graphics::DrawMesh(m_SphereMesh, {.position = {x, y, 0.0f}}, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        for (Vec3 const& pos : c_LightPositions) {
            Graphics::DrawMesh(m_SphereMesh, {.scale = Vec3{0.5f}, .position = pos}, m_PBRMaterial, m_Camera);
        }
    }

    ResourceLoader m_Loader = App::resource_loader();
    MouseCapturingCamera m_Camera = CreateCamera();
    Mesh m_SphereMesh = GenerateUVSphereMesh(64, 64);
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPBRLightingTexturedTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTexturedTab::LOGLPBRLightingTexturedTab(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab& osc::LOGLPBRLightingTexturedTab::operator=(LOGLPBRLightingTexturedTab&&) noexcept = default;
osc::LOGLPBRLightingTexturedTab::~LOGLPBRLightingTexturedTab() noexcept = default;

UID osc::LOGLPBRLightingTexturedTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRLightingTexturedTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRLightingTexturedTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRLightingTexturedTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRLightingTexturedTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRLightingTexturedTab::implOnDraw()
{
    m_Impl->onDraw();
}
