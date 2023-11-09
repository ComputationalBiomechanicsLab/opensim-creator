#include "LOGLBloomTab.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTarget.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <array>
#include <string>
#include <utility>
#include <vector>

using osc::Mat4;
using osc::Vec2;
using osc::Vec3;

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Bloom";

    constexpr auto c_SceneLightPositions = std::to_array<Vec3>(
    {
        { 0.0f, 0.5f,  1.5f},
        {-4.0f, 0.5f, -3.0f},
        { 3.0f, 0.5f,  1.0f},
        {-0.8f, 2.4f, -1.0f},
    });

    std::array<osc::Color, c_SceneLightPositions.size()> const& GetSceneLightColors()
    {
        static auto const s_SceneLightColors = std::to_array<osc::Color>(
        {
            osc::ToSRGB({ 5.0f, 5.0f,  5.0f}),
            osc::ToSRGB({10.0f, 0.0f,  0.0f}),
            osc::ToSRGB({ 0.0f, 0.0f, 15.0f}),
            osc::ToSRGB({ 0.0f, 5.0f,  0.0f}),
        });
        return s_SceneLightColors;
    }

    std::vector<Mat4> CreateCubeTransforms()
    {
        std::vector<Mat4> rv;

        {
            Mat4 m{1.0f};
            m = osc::Translate(m, Vec3(0.0f, 1.5f, 0.0));
            m = osc::Scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m{1.0f};
            m = osc::Translate(m, Vec3(2.0f, 0.0f, 1.0));
            m = osc::Scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m{1.0f};
            m = osc::Translate(m, Vec3(-1.0f, -1.0f, 2.0));
            m = osc::Rotate(m, osc::Deg2Rad(60.0f), osc::Normalize(Vec3(1.0, 0.0, 1.0)));
            rv.push_back(m);
        }

        {
            Mat4 m{1.0f};
            m = osc::Translate(m, Vec3(0.0f, 2.7f, 4.0));
            m = osc::Rotate(m, osc::Deg2Rad(23.0f), osc::Normalize(Vec3(1.0, 0.0, 1.0)));
            m = osc::Scale(m, Vec3(1.25));
            rv.push_back(m);
        }

        {
            Mat4 m(1.0f);
            m = osc::Translate(m, Vec3(-2.0f, 1.0f, -3.0));
            m = osc::Rotate(m, osc::Deg2Rad(124.0f), osc::Normalize(Vec3(1.0, 0.0, 1.0)));
            rv.push_back(m);
        }

        {
            Mat4 m(1.0f);
            m = osc::Translate(m, Vec3(-3.0f, 0.0f, 0.0));
            m = osc::Scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        return rv;
    }

    osc::Camera CreateCameraThatMatchesLearnOpenGL()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
        return rv;
    }
}

class osc::LOGLBloomTab::Impl final : public osc::StandardTabBase {
public:

    Impl() : StandardTabBase{c_TabStringID}
    {
        m_SceneMaterial.setVec3Array("uLightPositions", c_SceneLightPositions);
        m_SceneMaterial.setColorArray("uLightColors", GetSceneLightColors());
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
        m_IsMouseCaptured = false;
    }

    bool implOnEvent(SDL_Event const& e) final
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

    void implOnDraw() final
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

        reformatAllTextures(viewportRect);
        renderSceneMRT();
        renderBlurredBrightness();
        renderCombinedScene(viewportRect);
        drawOverlays(viewportRect);
    }

    void reformatAllTextures(Rect const& viewportRect)
    {
        Vec2 const viewportDims = Dimensions(viewportRect);
        AntiAliasingLevel const msxaaSamples = App::get().getCurrentAntiAliasingLevel();

        RenderTextureDescriptor textureDescription{viewportDims};
        textureDescription.setAntialiasingLevel(msxaaSamples);
        textureDescription.setColorFormat(RenderTextureFormat::DefaultHDR);

        // direct render targets are multisampled HDR textures
        m_SceneHDRColorOutput.reformat(textureDescription);
        m_SceneHDRThresholdedOutput.reformat(textureDescription);

        // intermediate buffers are single-sampled HDR textures
        textureDescription.setAntialiasingLevel(AntiAliasingLevel::none());
        for (RenderTexture& pingPongBuffer : m_PingPongBlurOutputBuffers)
        {
            pingPongBuffer.reformat(textureDescription);
        }
    }

    void renderSceneMRT()
    {
        drawSceneCubesToCamera();
        drawLightBoxesToCamera();
        flushCameraRenderQueueToMRT();
    }

    void drawSceneCubesToCamera()
    {
        m_SceneMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());

        // draw floor
        {
            Mat4 floorTransform{1.0f};
            floorTransform = osc::Translate(floorTransform, Vec3(0.0f, -1.0f, 0.0));
            floorTransform = osc::Scale(floorTransform, Vec3(12.5f, 0.5f, 12.5f));

            MaterialPropertyBlock floorProps;
            floorProps.setTexture("uDiffuseTexture", m_WoodTexture);

            Graphics::DrawMesh(
                m_CubeMesh,
                floorTransform,
                m_SceneMaterial,
                m_Camera,
                floorProps
            );
        }

        MaterialPropertyBlock cubeProps;
        cubeProps.setTexture("uDiffuseTexture", m_ContainerTexture);
        for (auto const& cubeTransform : CreateCubeTransforms())
        {
            Graphics::DrawMesh(
                m_CubeMesh,
                cubeTransform,
                m_SceneMaterial,
                m_Camera,
                cubeProps
            );
        }
    }

    void drawLightBoxesToCamera()
    {
        std::array<osc::Color, c_SceneLightPositions.size()> const& sceneLightColors = GetSceneLightColors();

        for (size_t i = 0; i < c_SceneLightPositions.size(); ++i)
        {
            Mat4 lightTransform{1.0f};
            lightTransform = osc::Translate(lightTransform, Vec3(c_SceneLightPositions[i]));
            lightTransform = osc::Scale(lightTransform, Vec3(0.25f));

            MaterialPropertyBlock lightProps;
            lightProps.setColor("uLightColor", sceneLightColors[i]);

            Graphics::DrawMesh(
                m_CubeMesh,
                lightTransform,
                m_LightboxMaterial,
                m_Camera,
                lightProps
            );
        }
    }

    void flushCameraRenderQueueToMRT()
    {
        RenderTarget mrt
        {
            {
                RenderTargetColorAttachment
                {
                    m_SceneHDRColorOutput.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
                RenderTargetColorAttachment
                {
                    m_SceneHDRThresholdedOutput.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
            },
            RenderTargetDepthAttachment
            {
                m_SceneHDRThresholdedOutput.updDepthBuffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };
        m_Camera.renderTo(mrt);
    }

    void renderBlurredBrightness()
    {
        m_BlurMaterial.setRenderTexture("uInputImage", m_SceneHDRThresholdedOutput);

        bool horizontal = false;
        for (RenderTexture& pingPongBuffer : m_PingPongBlurOutputBuffers)
        {
            m_BlurMaterial.setBool("uHorizontal", horizontal);
            Camera camera;
            Graphics::DrawMesh(m_QuadMesh, Transform{}, m_BlurMaterial, camera);
            camera.renderTo(pingPongBuffer);
            m_BlurMaterial.clearRenderTexture("uInputImage");

            horizontal = !horizontal;
        }
    }

    void renderCombinedScene(Rect const& viewportRect)
    {
        m_FinalCompositingMaterial.setRenderTexture("uHDRSceneRender", m_SceneHDRColorOutput);
        m_FinalCompositingMaterial.setRenderTexture("uBloomBlur", m_PingPongBlurOutputBuffers[0]);
        m_FinalCompositingMaterial.setBool("uBloom", true);
        m_FinalCompositingMaterial.setFloat("uExposure", 1.0f);

        Camera camera;
        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_FinalCompositingMaterial, camera);
        camera.setPixelRect(viewportRect);
        camera.renderToScreen();

        m_FinalCompositingMaterial.clearRenderTexture("uBloomBlur");
        m_FinalCompositingMaterial.clearRenderTexture("uHDRSceneRender");
    }

    void drawOverlays(Rect const& viewportRect)
    {
        constexpr float w = 200.0f;

        auto const textures = std::to_array<RenderTexture const*>(
        {
            &m_SceneHDRColorOutput,
            &m_SceneHDRThresholdedOutput,
            m_PingPongBlurOutputBuffers.data(),
            m_PingPongBlurOutputBuffers.data() + 1,
        });

        for (size_t i = 0; i < textures.size(); ++i)
        {
            Vec2 const offset = {static_cast<float>(i)*w, 0.0f};
            Rect const overlayRect
            {
                viewportRect.p1 + offset,
                viewportRect.p1 + offset + w
            };

            Graphics::BlitToScreen(*textures[i], overlayRect);
        }
    }

    Material m_SceneMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.frag"),
        },
    };

    Material m_LightboxMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.frag"),
        },
    };

    Material m_BlurMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.frag"),
        },
    };

    Material m_FinalCompositingMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.frag"),
        },
    };

    Texture2D m_WoodTexture = osc::LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Texture2D m_ContainerTexture = osc::LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/container2.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();

    RenderTexture m_SceneHDRColorOutput;
    RenderTexture m_SceneHDRThresholdedOutput;
    std::array<RenderTexture, 2> m_PingPongBlurOutputBuffers;

    Camera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    bool m_IsMouseCaptured = true;
    Vec3 m_CameraEulers = {};
};


// public API

osc::CStringView osc::LOGLBloomTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLBloomTab::LOGLBloomTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
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
