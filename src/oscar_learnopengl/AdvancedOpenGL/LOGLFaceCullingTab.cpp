#include "LOGLFaceCullingTab.h"

#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/FaceCulling";

    Mesh GenerateCubeSimilarlyToLOGL()
    {
        Mesh m = GenerateCubeMesh();
        m.transformVerts({.scale = Vec3{0.5f}});
        return m;
    }

    Material GenerateUVTestingTextureMappedMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.frag"),
        }};

        rv.setTexture("uTexture", LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/uv_checker.jpg"),
            ColorSpace::sRGB
        ));

        return rv;
    }

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

class osc::LOGLFaceCullingTab::Impl final : public StandardTabImpl {
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
        drawScene();
        draw2DUI();
    }

    void drawScene()
    {
        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());
        Graphics::DrawMesh(m_Cube, identity<Transform>(), m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        if (ui::Button("off")) {
            m_Material.setCullMode(CullMode::Off);
        }
        if (ui::Button("back")) {
            m_Material.setCullMode(CullMode::Back);
        }
        if (ui::Button("front")) {
            m_Material.setCullMode(CullMode::Front);
        }
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_Material = GenerateUVTestingTextureMappedMaterial(m_Loader);
    Mesh m_Cube = GenerateCubeSimilarlyToLOGL();
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
};


// public API

CStringView osc::LOGLFaceCullingTab::id()
{
    return c_TabStringID;
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab& osc::LOGLFaceCullingTab::operator=(LOGLFaceCullingTab&&) noexcept = default;
osc::LOGLFaceCullingTab::~LOGLFaceCullingTab() noexcept = default;

UID osc::LOGLFaceCullingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLFaceCullingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLFaceCullingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLFaceCullingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLFaceCullingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLFaceCullingTab::implOnDraw()
{
    m_Impl->onDraw();
}
