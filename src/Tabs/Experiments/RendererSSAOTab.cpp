#include "RendererSSAOTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/ColorSpace.hpp"
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
#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Graphics/TextureWrapMode.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace
{
    osc::Camera CreateCameraWithSameParamsAsLearnOpenGL()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(50.0f);
        rv.setBackgroundColor(osc::Color::black());
        return rv;
    }

    std::vector<glm::vec3> GenerateSampleKernel(size_t numSamples)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> zeroToOne{0.0f, 1.0f};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<glm::vec3> rv;
        rv.reserve(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            // scale samples such that they are more aligned to
            // the center of the kernel
            float scale = static_cast<float>(i)/numSamples;
            scale = glm::mix(0.1f, 1.0f, scale*scale);

            glm::vec3 sample = {minusOneToOne(rng), minusOneToOne(rng), minusOneToOne(rng)};
            sample = glm::normalize(sample);
            sample *= zeroToOne(rng);
            sample *= scale;

            rv.push_back(sample);
        }

        return rv;
    }

    std::vector<glm::vec4> GenerateNoiseTexturePixels(size_t numPixels)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<glm::vec4> rv;
        rv.reserve(numPixels);
        for (size_t i = 0; i < numPixels; ++i)
        {
            rv.push_back(glm::vec4
            {
                minusOneToOne(rng),
                minusOneToOne(rng),
                0.0f,  // rotate around z-axis in tangent space
                0.0f,  // ignored (Texture2D doesn't support RGB --> RGBA upload conversion)
            });
        }
        return rv;
    }

    template<typename T>
    nonstd::span<uint8_t const> ToByteSpan(nonstd::span<T const> vs)
    {
        return
        {
            reinterpret_cast<uint8_t const*>(vs.data()),
            reinterpret_cast<uint8_t const*>(vs.data() + vs.size())
        };
    }

    osc::Texture2D GenerateNoiseTexture(glm::ivec2 dimensions)
    {
        std::vector<glm::vec4> const pixels =
            GenerateNoiseTexturePixels(static_cast<size_t>(dimensions.x * dimensions.y));

        osc::Texture2D rv
        {
            dimensions,
            osc::TextureFormat::RGBAFloat,
            ToByteSpan<glm::vec4>(pixels),
            osc::ColorSpace::Linear,
        };

        rv.setFilterMode(osc::TextureFilterMode::Nearest);
        rv.setWrapMode(osc::TextureWrapMode::Repeat);

        return rv;
    }

    osc::Material LoadGBufferMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentSSAOGeometry.vert"),
                osc::App::slurp("shaders/ExperimentSSAOGeometry.frag"),
            },
        };
    }

    osc::RenderTexture RenderTextureWithColorFormat(osc::RenderTextureFormat f)
    {
        osc::RenderTexture rv;
        rv.setColorFormat(f);
        return rv;
    }

    osc::Material LoadSSAOMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentSSAOSSAO.vert"),
                osc::App::slurp("shaders/ExperimentSSAOSSAO.frag"),
            },
        };
    }

    osc::Material LoadBlurMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentSSAOBlur.vert"),
                osc::App::slurp("shaders/ExperimentSSAOBlur.frag"),
            },
        };
    }

    osc::Material LoadLightingMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentSSAOLighting.vert"),
                osc::App::slurp("shaders/ExperimentSSAOLighting.frag"),
            },
        };
    }
}

class osc::RendererSSAOTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " RendererSSAOTab";
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
private:
    void draw3DScene()
    {
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();
        glm::vec2 const viewportDims = Dimensions(viewportRect);
        int32_t const samples = App::get().getMSXAASamplesRecommended();

        // ensure textures/buffers have correct dimensions
        {
            m_GBuffer.reformat(viewportDims, samples);
            m_SSAO.reformat(viewportDims, samples);
            m_Blur.reformat(viewportDims, samples);
            m_Lighting.reformat(viewportDims, samples);
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
            Transform cubeTransform;
            cubeTransform.position = glm::vec3{0.0f, 7.0f, 0.0f};
            cubeTransform.scale = glm::vec3{7.5f};

            m_GBuffer.material.setBool("uInvertedNormals", true);

            Graphics::DrawMesh(
                m_CubeMesh,
                cubeTransform,
                m_GBuffer.material,
                m_Camera
            );
        }

        // render sphere
        {
            Transform modelTransform;
            modelTransform.position = glm::vec3{0.0f, 0.5f, 0.0f};

            m_GBuffer.material.setBool("uInvertedNormals", false);

            Graphics::DrawMesh(
                m_SphereMesh,
                modelTransform,
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
        m_SSAO.material.setVec2("uNoiseScale", Dimensions(viewportRect) / glm::vec2{m_NoiseTexture.getDimensions()});
        m_SSAO.material.setInt("uKernelSize", static_cast<int32_t>(m_SampleKernel.size()));
        m_SSAO.material.setFloat("uRadius", 0.5f);
        m_SSAO.material.setFloat("uBias", 0.025f);

        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_SSAO.material, m_Camera);
        m_Camera.renderTo(m_SSAO.outputTexture);

        m_SSAO.material.clearRenderTexture("uPositionTex");
        m_SSAO.material.clearRenderTexture("uNormalTex");
    }

    void renderBlurPass()
    {
        m_Blur.material.setRenderTexture("uSSAOTex", m_SSAO.outputTexture);

        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_Blur.material, m_Camera);
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

        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_Lighting.material, m_Camera);
        m_Camera.renderTo(m_Lighting.outputTexture);

        m_Lighting.material.clearRenderTexture("uPositionTex");
        m_Lighting.material.clearRenderTexture("uNormalTex");
        m_Lighting.material.clearRenderTexture("uAlbedoTex");
        m_Lighting.material.clearRenderTexture("uSSAOTex");
    }

    void drawOverlays(Rect const& viewportRect)
    {
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
        Graphics::BlitToScreen(
            m_SSAO.outputTexture,
            Rect{viewportRect.p1 + glm::vec2{600.0f, 0.0f}, viewportRect.p1 + glm::vec2{600.0f, 0.0f} + 200.0f}
        );
        Graphics::BlitToScreen(
            m_Blur.outputTexture,
            Rect{viewportRect.p1 + glm::vec2{800.0f, 0.0f}, viewportRect.p1 + glm::vec2{800.0f, 0.0f} + 200.0f}
        );
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    std::vector<glm::vec3> m_SampleKernel = GenerateSampleKernel(64);
    osc::Texture2D m_NoiseTexture = GenerateNoiseTexture({4, 4});
    glm::vec3 m_LightPosition = {2.0f, 4.0f, -2.0f};
    Color m_LightColor = {0.2f, 0.2f, 0.7f, 1.0f};

    Camera m_Camera = CreateCameraWithSameParamsAsLearnOpenGL();
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};

    Mesh m_SphereMesh = GenUntexturedUVSphere(32, 32);
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();

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

    struct SSAORenderingState final {
        Material material = LoadSSAOMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::RED);

        void reformat(glm::vec2 dims, int32_t samples)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(samples);
        }
    } m_SSAO;

    struct BlurRenderingState final {
        Material material = LoadBlurMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::RED);

        void reformat(glm::vec2 dims, int32_t samples)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(samples);
        }
    } m_Blur;

    struct LightingRenderingState final {
        Material material = LoadLightingMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);

        void reformat(glm::vec2 dims, int32_t samples)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(samples);
        }
    } m_Lighting;
};


// public API (PIMPL)

osc::CStringView osc::RendererSSAOTab::id() noexcept
{
    return "Renderer/SSAO";
}

osc::RendererSSAOTab::RendererSSAOTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererSSAOTab::RendererSSAOTab(RendererSSAOTab&&) noexcept = default;
osc::RendererSSAOTab& osc::RendererSSAOTab::operator=(RendererSSAOTab&&) noexcept = default;
osc::RendererSSAOTab::~RendererSSAOTab() noexcept = default;

osc::UID osc::RendererSSAOTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererSSAOTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererSSAOTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererSSAOTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererSSAOTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererSSAOTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererSSAOTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererSSAOTab::implOnDraw()
{
    m_Impl->onDraw();
}
