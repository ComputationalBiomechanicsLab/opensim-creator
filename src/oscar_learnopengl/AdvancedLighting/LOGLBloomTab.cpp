#include "LOGLBloomTab.h"

#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Bloom";

    constexpr auto c_SceneLightPositions = std::to_array<Vec3>({
        { 0.0f, 0.5f,  1.5f},
        {-4.0f, 0.5f, -3.0f},
        { 3.0f, 0.5f,  1.0f},
        {-0.8f, 2.4f, -1.0f},
    });

    std::array<Color, c_SceneLightPositions.size()> const& GetSceneLightColors()
    {
        static auto const s_SceneLightColors = std::to_array<Color>({
            ToSRGB({ 5.0f, 5.0f,  5.0f}),
            ToSRGB({10.0f, 0.0f,  0.0f}),
            ToSRGB({ 0.0f, 0.0f, 15.0f}),
            ToSRGB({ 0.0f, 5.0f,  0.0f}),
        });
        return s_SceneLightColors;
    }

    std::vector<Mat4> CreateCubeTransforms()
    {
        std::vector<Mat4> rv;
        rv.reserve(6);

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(0.0f, 1.5f, 0.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(2.0f, 0.0f, 1.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-1.0f, -1.0f, 2.0));
            m = rotate(m, 60_deg, UnitVec3{1.0, 0.0, 1.0});
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(0.0f, 2.7f, 4.0));
            m = rotate(m, 23_deg, UnitVec3{1.0, 0.0, 1.0});
            m = scale(m, Vec3(1.25));
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-2.0f, 1.0f, -3.0));
            m = rotate(m, 124_deg, UnitVec3{1.0, 0.0, 1.0});
            rv.push_back(m);
        }

        {
            Mat4 m = identity<Mat4>();
            m = translate(m, Vec3(-3.0f, 0.0f, 0.0));
            m = scale(m, Vec3(0.5f));
            rv.push_back(m);
        }

        return rv;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.5f, 5.0f});
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
        return rv;
    }
}

class osc::LOGLBloomTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_SceneMaterial.setVec3Array("uLightPositions", c_SceneLightPositions);
        m_SceneMaterial.setColorArray("uLightColors", GetSceneLightColors());
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
        draw3DScene();
    }

    void draw3DScene()
    {
        Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();

        reformatAllTextures(viewportRect);
        renderSceneMRT();
        renderBlurredBrightness();
        renderCombinedScene(viewportRect);
        drawOverlays(viewportRect);
    }

    void reformatAllTextures(Rect const& viewportRect)
    {
        Vec2 const viewportDims = dimensions(viewportRect);
        AntiAliasingLevel const msxaaSamples = App::get().getCurrentAntiAliasingLevel();

        RenderTextureDescriptor textureDescription{viewportDims};
        textureDescription.setAntialiasingLevel(msxaaSamples);
        textureDescription.setColorFormat(RenderTextureFormat::DefaultHDR);

        // direct render targets are multisampled HDR textures
        m_SceneHDRColorOutput.reformat(textureDescription);
        m_SceneHDRThresholdedOutput.reformat(textureDescription);

        // intermediate buffers are single-sampled HDR textures
        textureDescription.setAntialiasingLevel(AntiAliasingLevel::none());
        for (RenderTexture& pingPongBuffer : m_PingPongBlurOutputBuffers) {
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
            Mat4 floorTransform = identity<Mat4>();
            floorTransform = translate(floorTransform, Vec3(0.0f, -1.0f, 0.0));
            floorTransform = scale(floorTransform, Vec3(12.5f, 0.5f, 12.5f));

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
        for (auto const& cubeTransform : CreateCubeTransforms()) {
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
        std::array<Color, c_SceneLightPositions.size()> const& sceneLightColors = GetSceneLightColors();

        for (size_t i = 0; i < c_SceneLightPositions.size(); ++i) {
            Mat4 lightTransform = identity<Mat4>();
            lightTransform = translate(lightTransform, Vec3(c_SceneLightPositions[i]));
            lightTransform = scale(lightTransform, Vec3(0.25f));

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
        RenderTarget mrt{
            {
                RenderTargetColorAttachment{
                    m_SceneHDRColorOutput.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
                RenderTargetColorAttachment{
                    m_SceneHDRThresholdedOutput.updColorBuffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
            },
            RenderTargetDepthAttachment{
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
        for (RenderTexture& pingPongBuffer : m_PingPongBlurOutputBuffers) {
            m_BlurMaterial.setBool("uHorizontal", horizontal);
            Camera camera;
            Graphics::DrawMesh(m_QuadMesh, identity<Transform>(), m_BlurMaterial, camera);
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
        Graphics::DrawMesh(m_QuadMesh, identity<Transform>(), m_FinalCompositingMaterial, camera);
        camera.setPixelRect(viewportRect);
        camera.renderToScreen();

        m_FinalCompositingMaterial.clearRenderTexture("uBloomBlur");
        m_FinalCompositingMaterial.clearRenderTexture("uHDRSceneRender");
    }

    void drawOverlays(Rect const& viewportRect)
    {
        constexpr float w = 200.0f;

        auto const textures = std::to_array<RenderTexture const*>({
            &m_SceneHDRColorOutput,
            &m_SceneHDRThresholdedOutput,
            m_PingPongBlurOutputBuffers.data(),
            m_PingPongBlurOutputBuffers.data() + 1,
        });

        for (size_t i = 0; i < textures.size(); ++i) {
            Vec2 const offset = {static_cast<float>(i)*w, 0.0f};
            Rect const overlayRect{
                viewportRect.p1 + offset,
                viewportRect.p1 + offset + w,
            };

            Graphics::BlitToScreen(*textures[i], overlayRect);
        }
    }

    ResourceLoader m_Loader = App::resource_loader();

    Material m_SceneMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Bloom.frag"),
    }};

    Material m_LightboxMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/LightBox.frag"),
    }};

    Material m_BlurMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Blur.frag"),
    }};

    Material m_FinalCompositingMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/bloom/Final.frag"),
    }};

    Texture2D m_WoodTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/wood.png"),
        ColorSpace::sRGB
    );
    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/container2.png"),
        ColorSpace::sRGB
    );
    Mesh m_CubeMesh = GenerateCubeMesh();
    Mesh m_QuadMesh = GenerateTexturedQuadMesh();

    RenderTexture m_SceneHDRColorOutput;
    RenderTexture m_SceneHDRThresholdedOutput;
    std::array<RenderTexture, 2> m_PingPongBlurOutputBuffers;

    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
};


// public API

CStringView osc::LOGLBloomTab::id()
{
    return c_TabStringID;
}

osc::LOGLBloomTab::LOGLBloomTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLBloomTab::LOGLBloomTab(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab& osc::LOGLBloomTab::operator=(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab::~LOGLBloomTab() noexcept = default;

UID osc::LOGLBloomTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLBloomTab::implGetName() const
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
