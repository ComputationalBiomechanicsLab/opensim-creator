#include "LOGLFaceCullingTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <SDL_events.h>
#include <imgui.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>

using namespace osc::literals;
using osc::App;
using osc::ColorSpace;
using osc::CStringView;
using osc::GenerateCubeMesh;
using osc::LoadTexture2DFromImage;
using osc::Material;
using osc::Mesh;
using osc::MouseCapturingCamera;
using osc::Shader;
using osc::Transform;
using osc::Vec3;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/FaceCulling";

    Mesh GenerateCubeSimilarlyToLOGL()
    {
        Mesh m = GenerateCubeMesh();
        m.transformVerts({.scale = Vec3{0.5f}});
        return m;
    }

    Material GenerateUVTestingTextureMappedMaterial()
    {
        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/FaceCulling.frag"),
        }};

        rv.setTexture("uTexture", LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/uv_checker.jpg"),
            ColorSpace::sRGB
        ));

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
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        Graphics::DrawMesh(m_Cube, Identity<Transform>(), m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        if (ImGui::Button("off")) {
            m_Material.setCullMode(CullMode::Off);
        }
        if (ImGui::Button("back")) {
            m_Material.setCullMode(CullMode::Back);
        }
        if (ImGui::Button("front")) {
            m_Material.setCullMode(CullMode::Front);
        }
        ImGui::End();
    }

    Material m_Material = GenerateUVTestingTextureMappedMaterial();
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

osc::UID osc::LOGLFaceCullingTab::implGetID() const
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
