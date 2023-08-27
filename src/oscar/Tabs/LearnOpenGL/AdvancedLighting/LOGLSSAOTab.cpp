#include "LOGLSSAOTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/AntiAliasingLevel.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTarget.hpp"
#include "oscar/Graphics/RenderTargetColorAttachment.hpp"
#include "oscar/Graphics/RenderTargetDepthAttachment.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/RenderTextureDescriptor.hpp"
#include "oscar/Graphics/RenderTextureFormat.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Graphics/TextureFormat.hpp"
#include "oscar/Graphics/TextureFilterMode.hpp"
#include "oscar/Graphics/TextureWrapMode.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Panels/PerfPanel.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
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
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/SSAO";

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
            // scale antiAliasingLevel such that they are more aligned to
            // the center of the kernel
            float scale = static_cast<float>(i)/static_cast<float>(numSamples);
            scale = glm::mix(0.1f, 1.0f, scale*scale);

            glm::vec3 sample = {minusOneToOne(rng), minusOneToOne(rng), minusOneToOne(rng)};
            sample = glm::normalize(sample);
            sample *= zeroToOne(rng);
            sample *= scale;

            rv.push_back(sample);
        }

        return rv;
    }

    std::vector<osc::Color> GenerateNoiseTexturePixels(size_t numPixels)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<osc::Color> rv;
        rv.reserve(numPixels);
        for (size_t i = 0; i < numPixels; ++i)
        {
            rv.emplace_back(
                minusOneToOne(rng),
                minusOneToOne(rng),
                0.0f,  // rotate around z-axis in tangent space
                0.0f   // ignored (Texture2D doesn't support RGB --> RGBA upload conversion)
            );
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
        std::vector<osc::Color> const pixels =
            GenerateNoiseTexturePixels(static_cast<size_t>(dimensions.x) * static_cast<size_t>(dimensions.y));

        osc::Texture2D rv
        {
            dimensions,
            osc::TextureFormat::RGBAFloat,
            osc::ColorSpace::Linear,
            osc::TextureWrapMode::Repeat,
            osc::TextureFilterMode::Linear,
        };
        rv.setPixelData(ToByteSpan<osc::Color>(pixels));
        return rv;
    }

    osc::Material LoadGBufferMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Geometry.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Geometry.frag"),
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
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/SSAO.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/SSAO.frag"),
            },
        };
    }

    osc::Material LoadBlurMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Blur.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Blur.frag"),
            },
        };
    }

    osc::Material LoadLightingMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Lighting.vert"),
                osc::App::slurp("shaders/LearnOpenGL/AdvancedLighting/ssao/Lighting.frag"),
            },
        };
    }
}

class osc::LOGLSSAOTab::Impl final {
public:

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

        m_PerfPanel.onDraw();
    }
private:
    void draw3DScene()
    {
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();
        glm::vec2 const viewportDims = Dimensions(viewportRect);
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
        m_SSAO.material.setFloat("uBias", 0.125f);

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
        float const w = 200.0f;

        auto const textures = osc::to_array<RenderTexture const*>(
        {
            &m_GBuffer.albedo,
            &m_GBuffer.normal,
            &m_GBuffer.position,
            &m_SSAO.outputTexture,
            &m_Blur.outputTexture,
        });

        for (size_t i = 0; i < textures.size(); ++i)
        {
            glm::vec2 const offset = {static_cast<float>(i)*w, 0.0f};
            Rect const overlayRect{viewportRect.p1 + offset, viewportRect.p1 + offset + w};

            Graphics::BlitToScreen(*textures[i], overlayRect);
        }
    }

    UID m_TabID;

    std::vector<glm::vec3> m_SampleKernel = GenerateSampleKernel(64);
    Texture2D m_NoiseTexture = GenerateNoiseTexture({4, 4});
    glm::vec3 m_LightPosition = {2.0f, 4.0f, -2.0f};
    Color m_LightColor = {0.2f, 0.2f, 0.7f, 1.0f};

    Camera m_Camera = CreateCameraWithSameParamsAsLearnOpenGL();
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {};

    Mesh m_SphereMesh = GenSphere(32, 32);
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();

    // rendering state
    struct GBufferRenderingState final {
        Material material = LoadGBufferMaterial();
        RenderTexture albedo = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);
        RenderTexture normal = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTexture position = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTarget renderTarget
        {
            {
                RenderTargetColorAttachment
                {
                    albedo.updColorBuffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment
                {
                    normal.updColorBuffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment
                {
                    position.updColorBuffer(),
                    RenderBufferLoadAction::Load,
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

        void reformat(glm::vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            RenderTextureDescriptor desc{dims};
            desc.setAntialiasingLevel(antiAliasingLevel);

            for (RenderTexture* tex : {&albedo, &normal, &position})
            {
                desc.setColorFormat(tex->getColorFormat());
                tex->reformat(desc);
            }
        }
    } m_GBuffer;

    struct SSAORenderingState final {
        Material material = LoadSSAOMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(glm::vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_SSAO;

    struct BlurRenderingState final {
        Material material = LoadBlurMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(glm::vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_Blur;

    struct LightingRenderingState final {
        Material material = LoadLightingMaterial();
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);

        void reformat(glm::vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.setDimensions(dims);
            outputTexture.setAntialiasingLevel(antiAliasingLevel);
        }
    } m_Lighting;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API (PIMPL)

osc::CStringView osc::LOGLSSAOTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLSSAOTab::LOGLSSAOTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLSSAOTab::LOGLSSAOTab(LOGLSSAOTab&&) noexcept = default;
osc::LOGLSSAOTab& osc::LOGLSSAOTab::operator=(LOGLSSAOTab&&) noexcept = default;
osc::LOGLSSAOTab::~LOGLSSAOTab() noexcept = default;

osc::UID osc::LOGLSSAOTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLSSAOTab::implGetName() const
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
