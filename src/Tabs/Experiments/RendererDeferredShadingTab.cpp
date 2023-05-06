#include "RendererDeferredShadingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/RenderBufferLoadAction.hpp"
#include "src/Graphics/RenderBufferStoreAction.hpp"
#include "src/Graphics/RenderTarget.hpp"
#include "src/Graphics/RenderTargetColorAttachment.hpp"
#include "src/Graphics/RenderTargetDepthAttachment.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr glm::vec3 c_ObjectPositions[] =
    {
        {0.0f, 0.0f ,0.0f},
        {-3.0,  -0.5, -3.0},
        { 0.0,  -0.5, -3.0},
        { 3.0,  -0.5, -3.0},
        {-3.0,  -0.5,  0.0},
        { 0.0,  -0.5,  0.0},
        { 3.0,  -0.5,  0.0},
        {-3.0,  -0.5,  3.0},
        { 0.0,  -0.5,  3.0},
        { 3.0,  -0.5,  3.0},
    };

    struct SceneLight final {
        glm::vec3 pos;
        osc::Color color;
    };

    glm::vec3 GenerateSceneLightPosition(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{-3.0f, 3.0f};
        return {dist(rng), dist(rng), dist(rng)};
    }

    osc::Color GenerateSceneLightColor(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{0.5f, 1.0f};
        return {dist(rng), dist(rng), dist(rng), 1.0f};
    }

    SceneLight GenerateSceneLight(std::default_random_engine& rng)
    {
        return {GenerateSceneLightPosition(rng), GenerateSceneLightColor(rng)};
    }

    std::vector<SceneLight> GenerateNSceneLights(size_t n)
    {
        auto const generator = [rng = std::default_random_engine{}]() mutable
        {
            return GenerateSceneLight(rng);
        };

        std::vector<SceneLight> rv;
        rv.reserve(n);
        std::generate_n(std::back_inserter(rv), n, generator);
        return rv;
    }

    osc::Material LoadGBufferMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentDeferredShadingGBuffer.vert"),
                osc::App::slurp("shaders/ExperimentDeferredShadingGBuffer.frag"),
            },
        };
    }

    osc::RenderTexture RenderTextureWithColorFormat(osc::RenderTextureFormat f)
    {
        osc::RenderTexture rv;
        rv.setColorFormat(f);
        return rv;
    }
}

class osc::RendererDeferredShadingTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_Camera.setPosition({0.0f, 0.0f, 5.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor(Color::black());
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " RendererDeferredShadingTab";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();

        // capture the mouse when mounting this tab
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();

        // un-capture the mouse when un-mounting this tab
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
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onTick()
    {
    }

    void onDrawMainMenu()
    {
    }

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

    void draw3DScene()
    {
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();

        renderSceneToGBuffers(
            Dimensions(viewportRect),
            App::get().getMSXAASamplesRecommended()
        );

        // demo overlays: top left
        Graphics::BlitToScreen(
            m_GBuffer.albedo,
            Rect{viewportRect.p1, viewportRect.p1 + 200.0f}
        );
        Graphics::BlitToScreen(
            m_GBuffer.normal,
            Rect{viewportRect.p1 + glm::vec2{200.0f, 0.0f}, viewportRect.p1 + glm::vec2{200.0f, 0.0f} + 200.0f}
        );
        Graphics::BlitToScreen(
            m_GBuffer.position,
            Rect{viewportRect.p1 + glm::vec2{400.0f, 0.0f}, viewportRect.p1 + glm::vec2{400.0f, 0.0f} + 200.0f}
        );
    }

    void renderSceneToGBuffers(glm::vec2 dimensions, int32_t samples)
    {
        // ensure gbuffers have correct format
        m_GBuffer.reformat(dimensions, samples);

        // render scene cubes
        Transform transform;
        transform.scale = glm::vec3{0.5f};
        for (glm::vec3 const& objectPosition : c_ObjectPositions)
        {
            transform.position = objectPosition;
            Graphics::DrawMesh(m_CubeMesh, transform, m_GBuffer.material, m_Camera);
        }
        m_Camera.renderTo(m_GBuffer.renderTarget);
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    // scene state
    std::vector<SceneLight> m_Lights = GenerateNSceneLights(32);
    Camera m_Camera;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Mesh m_CubeMesh = GenCube();

    // rendering state
    struct GBufferRenderingState final {
        Material material = LoadGBufferMaterial();
        RenderTexture albedo = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);
        RenderTexture normal = RenderTextureWithColorFormat(RenderTextureFormat::ARGBHalf);
        RenderTexture position = RenderTextureWithColorFormat(RenderTextureFormat::ARGBHalf);
        RenderTarget renderTarget
        {
            {
                RenderTargetColorAttachment
                {
                    albedo.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment
                {
                    normal.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment
                {
                    position.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
            },
            RenderTargetDepthAttachment
            {
                albedo.updDepthBuffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };

        void reformat(glm::vec2 dims, int32_t samples)
        {
            RenderTextureDescriptor desc{dims};
            desc.setAntialiasingLevel(samples);

            for (RenderTexture* tex : {&albedo, &normal, &position})
            {
                desc.setColorFormat(tex->getColorFormat());
                tex->reformat(desc);
            }
        }
    } m_GBuffer;
};


// public API (PIMPL)

osc::CStringView osc::RendererDeferredShadingTab::id() noexcept
{
    return "Renderer/DeferredShading";
}

osc::RendererDeferredShadingTab::RendererDeferredShadingTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererDeferredShadingTab::RendererDeferredShadingTab(RendererDeferredShadingTab&&) noexcept = default;
osc::RendererDeferredShadingTab& osc::RendererDeferredShadingTab::operator=(RendererDeferredShadingTab&&) noexcept = default;
osc::RendererDeferredShadingTab::~RendererDeferredShadingTab() noexcept = default;

osc::UID osc::RendererDeferredShadingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererDeferredShadingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererDeferredShadingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererDeferredShadingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererDeferredShadingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererDeferredShadingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererDeferredShadingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererDeferredShadingTab::implOnDraw()
{
    m_Impl->onDraw();
}
