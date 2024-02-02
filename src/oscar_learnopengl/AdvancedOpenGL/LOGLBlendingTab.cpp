#include "LOGLBlendingTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>
#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <SDL_events.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc::literals;
using osc::CStringView;
using osc::Mesh;
using osc::MouseCapturingCamera;
using osc::Vec2;
using osc::Vec3;

namespace
{
    constexpr auto c_PlaneVertices = std::to_array<Vec3>({
        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},

        { 5.0f, -0.5f,  5.0f},
        {-5.0f, -0.5f, -5.0f},
        { 5.0f, -0.5f, -5.0f},
    });

    constexpr auto c_PlaneTexCoords = std::to_array<Vec2>({
        {2.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 2.0f},

        {2.0f, 0.0f},
        {0.0f, 2.0f},
        {2.0f, 2.0f},
    });
    constexpr auto c_PlaneIndices = std::to_array<uint16_t>({0, 2, 1, 3, 5, 4});

    constexpr auto c_TransparentVerts = std::to_array<Vec3>({
        {0.0f,  0.5f, 0.0f},
        {0.0f, -0.5f, 0.0f},
        {1.0f, -0.5f, 0.0f},

        {0.0f,  0.5f, 0.0f},
        {1.0f, -0.5f, 0.0f},
        {1.0f,  0.5f, 0.0f},
    });

    constexpr auto c_TransparentTexCoords = std::to_array<Vec2>({
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
    });

    constexpr auto c_TransparentIndices = std::to_array<uint16_t>({0, 1, 2, 3, 4, 5});

    constexpr auto c_WindowLocations = std::to_array<Vec3>({
        {-1.5f, 0.0f, -0.48f},
        { 1.5f, 0.0f,  0.51f},
        { 0.0f, 0.0f,  0.7f},
        {-0.3f, 0.0f, -2.3f},
        { 0.5f, 0.0f, -0.6},
    });

    constexpr CStringView c_TabStringID = "LearnOpenGL/Blending";

    Mesh GeneratePlane()
    {
        Mesh rv;
        rv.setVerts(c_PlaneVertices);
        rv.setTexCoords(c_PlaneTexCoords);
        rv.setIndices(c_PlaneIndices);
        return rv;
    }

    Mesh GenerateTransparent()
    {
        Mesh rv;
        rv.setVerts(c_TransparentVerts);
        rv.setTexCoords(c_TransparentTexCoords);
        rv.setIndices(c_TransparentIndices);
        return rv;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLBlendingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_BlendingMaterial.setTransparent(true);
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
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        // cubes
        {
            m_OpaqueMaterial.setTexture("uTexture", m_MarbleTexture);
            Graphics::DrawMesh(m_CubeMesh, {.position = {-1.0f, 0.0f, -1.0f}}, m_OpaqueMaterial, m_Camera);
            Graphics::DrawMesh(m_CubeMesh, {.position = { 1.0f, 0.0f, -1.0f}}, m_OpaqueMaterial, m_Camera);
        }

        // floor
        {
            m_OpaqueMaterial.setTexture("uTexture", m_MetalTexture);
            Graphics::DrawMesh(m_PlaneMesh, Identity<Transform>(), m_OpaqueMaterial, m_Camera);
        }

        // windows
        {
            m_BlendingMaterial.setTexture("uTexture", m_WindowTexture);
            for (Vec3 const& windowLocation : c_WindowLocations) {
                Graphics::DrawMesh(m_TransparentMesh, {.position = windowLocation}, m_BlendingMaterial, m_Camera);
            }
        }

        m_Camera.renderToScreen();

        // auxiliary UI
        m_LogViewer.onDraw();
        m_PerfPanel.onDraw();
    }

    Material m_OpaqueMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Blending.vert"),
        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Blending.frag"),
    }};
    Material m_BlendingMaterial = m_OpaqueMaterial;
    Mesh m_CubeMesh = GenerateLearnOpenGLCubeMesh();
    Mesh m_PlaneMesh = GeneratePlane();
    Mesh m_TransparentMesh = GenerateTransparent();
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    Texture2D m_MarbleTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/marble.jpg"),
        ColorSpace::sRGB
    );
    Texture2D m_MetalTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/metal.png"),
        ColorSpace::sRGB
    );
    Texture2D m_WindowTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/window.png"),
        ColorSpace::sRGB
    );
    LogViewerPanel m_LogViewer{"log"};
    PerfPanel m_PerfPanel{"perf"};
};


// public API

CStringView osc::LOGLBlendingTab::id()
{
    return c_TabStringID;
}

osc::LOGLBlendingTab::LOGLBlendingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBlendingTab::LOGLBlendingTab(LOGLBlendingTab&&) noexcept = default;
osc::LOGLBlendingTab& osc::LOGLBlendingTab::operator=(LOGLBlendingTab&&) noexcept = default;
osc::LOGLBlendingTab::~LOGLBlendingTab() noexcept = default;

osc::UID osc::LOGLBlendingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLBlendingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBlendingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBlendingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBlendingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBlendingTab::implOnDraw()
{
    m_Impl->onDraw();
}
