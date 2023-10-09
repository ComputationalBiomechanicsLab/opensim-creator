#include "LOGLFaceCullingTab.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/FaceCulling";

    osc::Mesh GenerateCubeSimilarlyToLOGL()
    {
        osc::Mesh m = osc::GenCube();
        m.transformVerts(osc::Transform{}.withScale(0.5f));
        return m;
    }

    osc::Material GenerateUVTestingTextureMappedMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/AdvancedOpenGL/FaceCulling.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedOpenGL/FaceCulling.frag"),
            },
        };

        rv.setTexture("uTexture", osc::LoadTexture2DFromImage(
            osc::App::resource("textures/uv_checker.jpg"),
            osc::ColorSpace::sRGB
        ));

        return rv;
    }

    osc::Camera CreateCameraThatMatchesLearnOpenGL()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }
}

class osc::LOGLFaceCullingTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
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
        handleMouseInputs();
        drawScene();
        draw2DUI();
    }

    void handleMouseInputs()
    {
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
    }

    void drawScene()
    {
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        Graphics::DrawMesh(m_Cube, Transform{}, m_Material, m_Camera);
        m_Camera.renderToScreen();
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        if (ImGui::Button("off"))
        {
            m_Material.setCullMode(CullMode::Off);
        }
        if (ImGui::Button("back"))
        {
            m_Material.setCullMode(CullMode::Back);
        }
        if (ImGui::Button("front"))
        {
            m_Material.setCullMode(CullMode::Front);
        }
        ImGui::End();
    }

    Material m_Material = GenerateUVTestingTextureMappedMaterial();
    Mesh m_Cube = GenerateCubeSimilarlyToLOGL();
    Camera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    bool m_IsMouseCaptured = false;
    glm::vec3 m_CameraEulers = {};
};


// public API

osc::CStringView osc::LOGLFaceCullingTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLFaceCullingTab::LOGLFaceCullingTab(ParentPtr<TabHost> const&) :
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

osc::CStringView osc::LOGLFaceCullingTab::implGetName() const
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
