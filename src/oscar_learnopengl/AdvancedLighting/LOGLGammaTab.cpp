#include "LOGLGammaTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <imgui.h>
#include <oscar/oscar.hpp>
#include <SDL_events.h>

#include <array>
#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr auto c_PlaneVertices = std::to_array<Vec3>({
        { 10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f, -10.0f},

        { 10.0f, -0.5f,  10.0f},
        {-10.0f, -0.5f, -10.0f},
        { 10.0f, -0.5f, -10.0f},
    });
    constexpr auto c_PlaneTexCoords = std::to_array<Vec2>({
        {10.0f, 0.0f},
        {0.0f,  0.0f},
        {0.0f,  10.0f},

        {10.0f, 0.0f},
        {0.0f,  10.0f},
        {10.0f, 10.0f},
    });
    constexpr auto c_PlaneNormals = std::to_array<Vec3>({
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},

        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    constexpr auto c_PlaneIndices = std::to_array<uint16_t>({0, 2, 1, 3, 5, 4});

    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-3.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f},
        { 3.0f, 0.0f, 0.0f},
    });

    constexpr auto c_LightColors = std::to_array<Color>({
        {0.25f, 0.25f, 0.25f, 1.0f},
        {0.50f, 0.50f, 0.50f, 1.0f},
        {0.75f, 0.75f, 0.75f, 1.0f},
        {1.00f, 1.00f, 1.00f, 1.0f},
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/Gamma";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setNormals(c_PlaneNormals);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }

    MouseCapturingCamera CreateSceneCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    Material CreateFloorMaterial()
    {
        Texture2D woodTexture = LoadTexture2DFromImage(
            App::load_resource("oscar_learnopengl/textures/wood.png"),
            ColorSpace::sRGB
        );

        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/Gamma.frag"),
        }};
        rv.setTexture("uFloorTexture", woodTexture);
        rv.setVec3Array("uLightPositions", c_LightPositions);
        rv.setColorArray("uLightColors", c_LightColors);
        return rv;
    }
}

class osc::LOGLGammaTab::Impl final : public StandardTabImpl {
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
        draw3DScene();
        draw2DUI();
    }

    void draw3DScene()
    {
        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        // render scene
        m_Material.setVec3("uViewPos", m_Camera.getPosition());
        Graphics::DrawMesh(m_PlaneMesh, Identity<Transform>(), m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        ImGui::Text("no need to gamma correct - OSC is a gamma-corrected renderer");
        ImGui::End();
    }

    Material m_Material = CreateFloorMaterial();
    Mesh m_PlaneMesh = GeneratePlane();
    MouseCapturingCamera m_Camera = CreateSceneCamera();
};


// public API

CStringView osc::LOGLGammaTab::id()
{
    return c_TabStringID;
}

osc::LOGLGammaTab::LOGLGammaTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLGammaTab::LOGLGammaTab(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab& osc::LOGLGammaTab::operator=(LOGLGammaTab&&) noexcept = default;
osc::LOGLGammaTab::~LOGLGammaTab() noexcept = default;

UID osc::LOGLGammaTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLGammaTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLGammaTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLGammaTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLGammaTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLGammaTab::implOnDraw()
{
    m_Impl->onDraw();
}
