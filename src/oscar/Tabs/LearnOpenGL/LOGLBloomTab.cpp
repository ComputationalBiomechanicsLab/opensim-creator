#include "LOGLBloomTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/Algorithms.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Bloom";

    struct SceneLight final {
        glm::vec3 position;
        osc::Color color;
    };

    constexpr auto c_SceneLights = osc::MakeArray<SceneLight>(
        SceneLight{{ 0.0f, 0.5f,  1.5f}, { 5.0f, 5.0f,  5.0f}},
        SceneLight{{-4.0f, 0.5f, -3.0f}, {10.0f, 0.0f,  0.0f}},
        SceneLight{{ 3.0f, 0.5f,  1.0f}, { 0.0f, 0.0f, 15.0f}},
        SceneLight{{-0.8f, 2.4f, -1.0f}, { 0.0f, 5.0f,  0.0f}}
    );
}

class osc::LOGLBloomTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_Camera.setPosition({0.0f, 0.0f, 5.0f});
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
        m_IsMouseCaptured = false;
    }

    bool onEvent(SDL_Event const& e)
    {
        // handle mouse input
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onTick() {}
    void onDrawMainMenu() {}

    void onDraw()
    {
        // handle mouse capturing
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

        draw3DScene();
    }

private:
    void draw3DScene()
    {
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();

        reformatAllTextures(viewportRect);
        renderSceneMRT();
        renderBlurredBrightness();
        renderCombinedScene();
        blitCombinedSceneToScreen(viewportRect);
        drawOverlays(viewportRect);
    }

    void reformatAllTextures(Rect const&)
    {
        //glm::vec2 const viewportDims = Dimensions(viewportRect);
        //int32_t const samples = App::get().getMSXAASamplesRecommended();

        // TODO: reformat them all
    }

    void renderSceneMRT()
    {
        // TODO: render the cubes + lights via an MRT shader that writes
        // the scene and the thresholded brightness in one pass
    }

    void renderBlurredBrightness()
    {
        // TODO: pump thresholded output through blurring shader
    }

    void renderCombinedScene()
    {
        // TODO: composite the scene render and the blurred brightness into one scene
    }

    void blitCombinedSceneToScreen(Rect const&)
    {
        // TODO: blit the final render texture to the screen
    }

    void drawOverlays(Rect const&)
    {
        // TODO: use Graphics::BlitToScreen to draw each intermediate texture
        // to the corner of the viewport (see SSAO impl)
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    Shader m_SceneShader
    {
        App::slurp("shaders/ExperimentBloom.vert"),
        App::slurp("shaders/ExperimentBloom.frag"),
    };
    Shader m_LightboxShader
    {
        App::slurp("shaders/ExperimentBloomLightBox.vert"),
        App::slurp("shaders/ExperimentBloomLightBox.frag"),
    };
    Shader m_BlurShader
    {
        App::slurp("shaders/ExperimentBloomBlur.vert"),
        App::slurp("shaders/ExperimentBloomBlur.frag"),
    };
    Shader m_FinalShader
    {
        App::slurp("shaders/ExperimentBloomFinal.vert"),
        App::slurp("shaders/ExperimentBloomFinal.frag"),
    };
    Texture2D m_WoodTexture = osc::LoadTexture2DFromImage(
        App::resource("textures/wood.png"),
        ColorSpace::sRGB
    );
    Texture2D m_ContainerTexture = osc::LoadTexture2DFromImage(
        App::resource("textures/container2.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = osc::GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();

    Camera m_Camera;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};

    bool m_UseBloom = true;
    float m_Exposure = 1.0f;
};


// public API (PIMPL)

osc::CStringView osc::LOGLBloomTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLBloomTab::LOGLBloomTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::LOGLBloomTab::LOGLBloomTab(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab& osc::LOGLBloomTab::operator=(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab::~LOGLBloomTab() noexcept = default;

osc::UID osc::LOGLBloomTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLBloomTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBloomTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBloomTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBloomTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBloomTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLBloomTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLBloomTab::implOnDraw()
{
    m_Impl->onDraw();
}
