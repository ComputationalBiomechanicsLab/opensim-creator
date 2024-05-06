#include "LOGLSSAOTab.h"

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
        rv.set_position({0.0f, 0.0f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(50.0f);
        rv.set_background_color(Color::black());
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
        rv.set_pixel_data(view_object_representations<uint8_t>(pixels));
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
        rv.set_color_format(f);
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
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.on_mount();
    }

    void impl_on_unmount() final
    {
        m_Camera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool impl_on_event(SDL_Event const& e) final
    {
        return m_Camera.on_event(e);
    }

    void impl_on_draw() final
    {
        m_Camera.on_draw();
        draw3DScene();
        m_PerfPanel.on_draw();
    }

    void draw3DScene()
    {
        Rect const viewportRect = ui::GetMainViewportWorkspaceScreenRect();
        Vec2 const viewportDims = dimensions_of(viewportRect);
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
        graphics::blit_to_screen(m_Lighting.outputTexture, viewportRect);
        drawOverlays(viewportRect);
    }

    void renderGeometryPassToGBuffers()
    {
        // render cube
        {
            m_GBuffer.material.set_bool("uInvertedNormals", true);
            graphics::draw(
                m_CubeMesh,
                {.scale = Vec3{7.5f}, .position = {0.0f, 7.0f, 0.0f}},
                m_GBuffer.material,
                m_Camera
            );
        }

        // render sphere
        {
            m_GBuffer.material.set_bool("uInvertedNormals", false);
            graphics::draw(
                m_SphereMesh,
                {.position = {0.0f, 0.5f, 0.0f}},
                m_GBuffer.material,
                m_Camera
            );
        }

        m_Camera.render_to(m_GBuffer.renderTarget);
    }

    void renderSSAOPass(Rect const& viewportRect)
    {
        m_SSAO.material.set_render_texture("uPositionTex", m_GBuffer.position);
        m_SSAO.material.set_render_texture("uNormalTex", m_GBuffer.normal);
        m_SSAO.material.set_texture("uNoiseTex", m_NoiseTexture);
        m_SSAO.material.set_vec3_array("uSamples", m_SampleKernel);
        m_SSAO.material.set_vec2("uNoiseScale", dimensions_of(viewportRect) / Vec2{m_NoiseTexture.dimensions()});
        m_SSAO.material.set_int("uKernelSize", static_cast<int32_t>(m_SampleKernel.size()));
        m_SSAO.material.set_float("uRadius", 0.5f);
        m_SSAO.material.set_float("uBias", 0.125f);

        graphics::draw(m_QuadMesh, identity<Transform>(), m_SSAO.material, m_Camera);
        m_Camera.render_to(m_SSAO.outputTexture);

        m_SSAO.material.clear_render_texture("uPositionTex");
        m_SSAO.material.clear_render_texture("uNormalTex");
    }

    void renderBlurPass()
    {
        m_Blur.material.set_render_texture("uSSAOTex", m_SSAO.outputTexture);

        graphics::draw(m_QuadMesh, identity<Transform>(), m_Blur.material, m_Camera);
        m_Camera.render_to(m_Blur.outputTexture);

        m_Blur.material.clear_render_texture("uSSAOTex");
    }

    void renderLightingPass()
    {
        m_Lighting.material.set_render_texture("uPositionTex", m_GBuffer.position);
        m_Lighting.material.set_render_texture("uNormalTex", m_GBuffer.normal);
        m_Lighting.material.set_render_texture("uAlbedoTex", m_GBuffer.albedo);
        m_Lighting.material.set_render_texture("uSSAOTex", m_SSAO.outputTexture);
        m_Lighting.material.set_vec3("uLightPosition", m_LightPosition);
        m_Lighting.material.set_color("uLightColor", m_LightColor);
        m_Lighting.material.set_float("uLightLinear", 0.09f);
        m_Lighting.material.set_float("uLightQuadratic", 0.032f);

        graphics::draw(m_QuadMesh, identity<Transform>(), m_Lighting.material, m_Camera);
        m_Camera.render_to(m_Lighting.outputTexture);

        m_Lighting.material.clear_render_texture("uPositionTex");
        m_Lighting.material.clear_render_texture("uNormalTex");
        m_Lighting.material.clear_render_texture("uAlbedoTex");
        m_Lighting.material.clear_render_texture("uSSAOTex");
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

            graphics::blit_to_screen(*textures[i], overlayRect);
        }
    }

    std::vector<Vec3> m_SampleKernel = GenerateSampleKernel(64);
    Texture2D m_NoiseTexture = GenerateNoiseTexture({4, 4});
    Vec3 m_LightPosition = {2.0f, 4.0f, -2.0f};
    Color m_LightColor = {0.2f, 0.2f, 0.7f, 1.0f};

    MouseCapturingCamera m_Camera = CreateCameraWithSameParamsAsLearnOpenGL();

    Mesh m_SphereMesh = SphereGeometry{1.0f, 32, 32};
    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f};

    // rendering state
    struct GBufferRenderingState final {
        Material material = LoadGBufferMaterial(App::resource_loader());
        RenderTexture albedo = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);
        RenderTexture normal = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTexture position = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTarget renderTarget{
            {
                RenderTargetColorAttachment{
                    albedo.upd_color_buffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    normal.upd_color_buffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    position.upd_color_buffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
            },
            RenderTargetDepthAttachment{
                albedo.upd_depth_buffer(),
                RenderBufferLoadAction::Clear,
                RenderBufferStoreAction::DontCare,
            },
        };

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            RenderTextureDescriptor desc{dims};
            desc.set_anti_aliasing_level(antiAliasingLevel);

            for (RenderTexture* tex : {&albedo, &normal, &position}) {
                desc.set_color_format(tex->color_format());
                tex->reformat(desc);
            }
        }
    } m_GBuffer;

    struct SSAORenderingState final {
        Material material = LoadSSAOMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.set_dimensions(dims);
            outputTexture.set_anti_aliasing_level(antiAliasingLevel);
        }
    } m_SSAO;

    struct BlurRenderingState final {
        Material material = LoadBlurMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::Red8);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.set_dimensions(dims);
            outputTexture.set_anti_aliasing_level(antiAliasingLevel);
        }
    } m_Blur;

    struct LightingRenderingState final {
        Material material = LoadLightingMaterial(App::resource_loader());
        RenderTexture outputTexture = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);

        void reformat(Vec2 dims, AntiAliasingLevel antiAliasingLevel)
        {
            outputTexture.set_dimensions(dims);
            outputTexture.set_anti_aliasing_level(antiAliasingLevel);
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

UID osc::LOGLSSAOTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::LOGLSSAOTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::LOGLSSAOTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::LOGLSSAOTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLSSAOTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->on_event(e);
}

void osc::LOGLSSAOTab::impl_on_draw()
{
    m_Impl->on_draw();
}
