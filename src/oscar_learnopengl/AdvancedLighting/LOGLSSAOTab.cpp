#include "LOGLSSAOTab.h"

#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/SSAO";

    MouseCapturingCamera CreateCameraWithSameParamsAsLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(50.0f);
        rv.setBackgroundColor(Color::black());
        return rv;
    }

    std::vector<Vec3> GenerateSampleKernel(size_t numSamples)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> zeroToOne{0.0f, 1.0f};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<Vec3> rv;
        rv.reserve(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            // scale antiAliasingLevel such that they are more aligned to
            // the center of the kernel
            float scale = static_cast<float>(i)/static_cast<float>(numSamples);
            scale = lerp(0.1f, 1.0f, scale*scale);

            Vec3 sample = {minusOneToOne(rng), minusOneToOne(rng), minusOneToOne(rng)};
            sample = normalize(sample);
            sample *= zeroToOne(rng);
            sample *= scale;

            rv.push_back(sample);
        }

        return rv;
    }

    std::vector<Color> GenerateNoiseTexturePixels(size_t numPixels) {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<Color> rv;
        rv.reserve(numPixels);
        for (size_t i = 0; i < numPixels; ++i) {
            rv.emplace_back(
                minusOneToOne(rng),
                minusOneToOne(rng),
                0.0f,  // rotate around z-axis in tangent space
                0.0f   // ignored (Texture2D doesn't support RGB --> RGBA upload conversion)
            );
        }
        return rv;
    }

    Texture2D GenerateNoiseTexture(Vec2i dimensions)
    {
        std::vector<Color> const pixels =
            GenerateNoiseTexturePixels(static_cast<size_t>(dimensions.x) * static_cast<size_t>(dimensions.y));

        Texture2D rv{
            dimensions,
            TextureFormat::RGBAFloat,
            ColorSpace::Linear,
            TextureWrapMode::Repeat,
            TextureFilterMode::Linear,
        };
        rv.setPixelData(ViewObjectRepresentations<uint8_t>(pixels));
        return rv;
    }

    Material LoadGBufferMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Geometry.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Geometry.frag"),
        }};
    }

    RenderTexture RenderTextureWithColorFormat(RenderTextureFormat f)
    {
        RenderTexture rv;
        rv.setColorFormat(f);
        return rv;
    }

    Material LoadSSAOMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/SSAO.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/SSAO.frag"),
        }};
    }

    Material LoadBlurMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Blur.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Blur.frag"),
        }};
    }

    Material LoadLightingMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Lighting.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/ssao/Lighting.frag"),
        }};
    }
}

class osc::LOGLSSAOTab::Impl final : public StandardTabImpl {
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
        m_PerfPanel.onDraw();
    }

    void draw3DScene()
    {
        Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();
        Vec2 const viewportDims = dimensions(viewportRect);
        AntiAliasingLevel const antiAliasingLevel = AntiAliasingLevel::none();

        // ensure textures/buffers have correct dimensions
        {
            m_GBuffer.reformat(viewportDims, antiAliasingLevel);
            m_SSAO.reformat(viewportDims, antiAliasingLevel);
            m_Blur.reformat(viewportDims, antiAliasingLevel);
            m_Lighting.reformat(viewportDims, antiAliasingLevel);
        }

        renderGeometryPassToGBuffers();
        renderSSAOPass(viewportRect);
        renderBlurPass();
        renderLightingPass();
        Graphics::BlitToScreen(m_Lighting.outputTexture, viewportRect);
        drawOverlays(viewportRect);
    }

    void renderGeometryPassToGBuffers()
    {
        // render cube
        {
            m_GBuffer.material.setBool("uInvertedNormals", true);
            Graphics::DrawMesh(
                m_CubeMesh,
                {.scale = Vec3{7.5f}, .position = {0.0f, 7.0f, 0.0f}},
                m_GBuffer.material,
                m_Camera
            );
        }

        // render sphere
        {
            m_GBuffer.material.setBool("uInvertedNormals", false);
            Graphics::DrawMesh(
                m_SphereMesh,
                {.position = {0.0f, 0.5f, 0.0f}},
                m_GBuffer.material,
                m_Camera
            );
        }

        m_Camera.renderTo(m_GBuffer.renderTarget);
    }

    void renderSSAOPass(Rect const& viewportRect)
    {
        m_SSAO.material.setRenderTexture("uPositionTex", m_GBuffer.position);
        m_SSAO.material.setRenderTexture("uNormalTex", m_GBuffer.normal);
        m_SSAO.material.setTexture("uNoiseTex", m_NoiseTexture);
        m_SSAO.material.setVec3Array("uSamples", m_SampleKernel);
        m_SSAO.material.setVec2("uNoiseScale", dimensions(viewportRect) / Vec2{m_NoiseTexture.getDimensions()});
        m_SSAO.material.setInt("uKernelSize", static_cast<int32_t>(m_SampleKernel.size()));
        m_SSAO.material.setFloat("uRadius", 0.5f);
        m_SSAO.material.setFloat("uBias", 0.125f);

        Graphics::DrawMesh(m_QuadMesh, identity<Transform>(), m_SSAO.material, m_Camera);
        m_Camera.renderTo(m_SSAO.outputTexture);

        m_SSAO.material.clearRenderTexture("uPositionTex");
        m_SSAO.material.clearRenderTexture("uNormalTex");
    }

    void renderBlurPass()
    {
        m_Blur.material.setRenderTexture("uSSAOTex", m_SSAO.outputTexture);

        Graphics::DrawMesh(m_QuadMesh, identity<Transform>(), m_Blur.material, m_Camera);
        m_Camera.renderTo(m_Blur.outputTexture);

        m_Blur.material.clearRenderTexture("uSSAOTex");
    }

    void renderLightingPass()
    {
        m_Lighting.material.setRenderTexture("uPositionTex", m_GBuffer.position);
        m_Lighting.material.setRenderTexture("uNormalTex", m_GBuffer.normal);
        m_Lighting.material.setRenderTexture("uAlbedoTex", m_GBuffer.albedo);
        m_Lighting.material.setRenderTexture("uSSAOTex", m_SSAO.outputTexture);
        m_Lighting.material.setVec3("uLightPosition", m_LightPosition);
        m_Lighting.material.setColor("uLightColor", m_LightColor);
        m_Lighting.material.setFloat("uLightLinear", 0.09f);
        m_Lighting.material.setFloat("uLightQuadratic", 0.032f);

        Graphics::DrawMesh(m_QuadMesh, identity<Transform>(), m_Lighting.material, m_Camera);
        m_Camera.renderTo(m_Lighting.outputTexture);

        m_Lighting.material.clearRenderTexture("uPositionTex");
        m_Lighting.material.clearRenderTexture("uNormalTex");
        m_Lighting.material.clearRenderTexture("uAlbedoTex");
        m_Lighting.material.clearRenderTexture("uSSAOTex");
    }

    void drawOverlays(Rect const& viewportRect)
    {
        float const w = 200.0f;

        auto const textures = std::to_array<RenderTexture const*>({
            &m_GBuffer.albedo,
            &m_GBuffer.normal,
            &m_GBuffer.position,
            &m_SSAO.outputTexture,
            &m_Blur.outputTexture,
        });

        for (size_t i = 0; i < textures.size(); ++i) {
            Vec2 const offset = {static_cast<float>(i)*w, 0.0f};
            Rect const overlayRect{viewportRect.p1 + offset, viewportRect.p1 + offset + w};

            Graphics::BlitToScreen(*textures[i], overlayRect);
        }
    }

    std::vector<Vec3> m_SampleKernel = GenerateSampleKernel(64);
    Texture2D m_NoiseTexture = GenerateNoiseTexture({4, 4});
    Vec3 m_LightPosition = {2.0f, 4.0f, -2.0f};
    Color m_LightColor = {0.2f, 0.2f, 0.7f, 1.0f};

    MouseCapturingCamera m_Camera = CreateCameraWithSameParamsAsLearnOpenGL();

    Mesh m_SphereMesh = GenerateSphereMesh2(1.0f, 32, 32);
    Mesh m_CubeMesh = GenerateBoxMesh(2.0f, 2.0f, 2.0f);
    Mesh m_QuadMesh = GeneratePlaneMesh2(2.0f, 2.0f);

    // rendering state
    struct GBufferRenderingState final {
        Material material = LoadGBufferMaterial(App::resource_loader());
        RenderTexture albedo = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);
        RenderTexture normal = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTexture position = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTarget renderTarget{
            {
                RenderTargetColorAttachment{
                    albedo.updColorBuffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    normal.updColorBuffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    position.updColorBuffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
            },
            RenderTargetDepthAttachment{
                albedo.updDepthBuffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            RenderTextureDescriptor desc{dims};
            desc.setAntialiasingLevel(antiAliasingLevel);

            for (RenderTexture* tex : {&albedo, &normal, &position}) {
                desc.setColorFormat(tex->getColorFormat());
                tex->reformat(desc);
            }
        }
    } m_GBuffer;

    struct SSAORenderingState final {
        Material material = LoadSSAOMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_SSAO;

    struct BlurRenderingState final {
        Material material = LoadBlurMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_Blur;

    struct LightingRenderingState final {
        Material material = LoadLightingMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_Lighting;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API (PIMPL)

CStringView osc::LOGLSSAOTab::id()
{
    return c_TabStringID;
}

osc::LOGLSSAOTab::LOGLSSAOTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLSSAOTab::LOGLSSAOTab(LOGLSSAOTab&&) noexcept = default;
osc::LOGLSSAOTab& osc::LOGLSSAOTab::operator=(LOGLSSAOTab&&) noexcept = default;
osc::LOGLSSAOTab::~LOGLSSAOTab() noexcept = default;

UID osc::LOGLSSAOTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLSSAOTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLSSAOTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLSSAOTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLSSAOTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLSSAOTab::implOnDraw()
{
    m_Impl->onDraw();
}
