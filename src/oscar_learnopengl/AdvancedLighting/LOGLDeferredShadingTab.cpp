#include "LOGLDeferredShadingTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/DeferredShading";

    constexpr auto c_ObjectPositions = std::to_array<Vec3>({
        {-3.0,  -0.5, -3.0},
        { 0.0,  -0.5, -3.0},
        { 3.0,  -0.5, -3.0},
        {-3.0,  -0.5,  0.0},
        { 0.0,  -0.5,  0.0},
        { 3.0,  -0.5,  0.0},
        {-3.0,  -0.5,  3.0},
        { 0.0,  -0.5,  3.0},
        { 3.0,  -0.5,  3.0},
    });
    constexpr size_t c_NumLights = 32;

    Vec3 GenerateSceneLightPosition(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{-3.0f, 3.0f};
        return {dist(rng), dist(rng), dist(rng)};
    }

    Color GenerateSceneLightColor(std::default_random_engine& rng)
    {
        std::uniform_real_distribution<float> dist{0.5f, 1.0f};
        return {dist(rng), dist(rng), dist(rng), 1.0f};
    }

    std::vector<Vec3> GenerateNSceneLightPositions(size_t n)
    {
        auto const generator = [rng = std::default_random_engine{std::random_device{}()}]() mutable
        {
            return GenerateSceneLightPosition(rng);
        };

        std::vector<Vec3> rv;
        rv.reserve(n);
        std::generate_n(std::back_inserter(rv), n, generator);
        return rv;
    }

    std::vector<Vec3> GenerateNSceneLightColors(size_t n)
    {
        auto const generator = [rng = std::default_random_engine{std::random_device{}()}]() mutable
        {
            Color const sRGBColor = GenerateSceneLightColor(rng);
            Color const linearColor = to_linear_colorspace(sRGBColor);
            return Vec3{linearColor.r, linearColor.g, linearColor.b};
        };

        std::vector<Vec3> rv;
        rv.reserve(n);
        std::generate_n(std::back_inserter(rv), n, generator);
        return rv;
    }

    Material LoadGBufferMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/GBuffer.vert"),
            rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/GBuffer.frag"),
        }};
    }

    RenderTexture RenderTextureWithColorFormat(RenderTextureFormat f)
    {
        RenderTexture rv;
        rv.set_color_format(f);
        return rv;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.5f, 5.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color(Color::black());
        return rv;
    }

    struct GBufferRenderingState final {

        explicit GBufferRenderingState(IResourceLoader& rl) :
            material{LoadGBufferMaterial(rl)}
        {}

        Material material;
        RenderTexture albedo = RenderTextureWithColorFormat(RenderTextureFormat::ARGB32);
        RenderTexture normal = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTexture position = RenderTextureWithColorFormat(RenderTextureFormat::ARGBFloat16);
        RenderTarget renderTarget{
            {
                RenderTargetColorAttachment{
                    albedo.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    normal.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
                    RenderBufferStoreAction::Resolve,
                    Color::black(),
                },
                RenderTargetColorAttachment{
                    position.upd_color_buffer(),
                    RenderBufferLoadAction::Clear,
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
    };

    struct LightPassState final {

        explicit LightPassState(IResourceLoader& rl) :
            material{Shader{
                rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightingPass.vert"),
                rl.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightingPass.frag"),
            }}
        {}

        Material material;
    };
}

class osc::LOGLDeferredShadingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
        App::upd().make_main_loop_waiting();
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
        Vec2 const viewportDims = dimensions_of(viewportRect);
        AntiAliasingLevel const antiAliasingLevel = App::get().anti_aliasing_level();

        // ensure textures/buffers have correct dimensions
        {
            m_GBuffer.reformat(viewportDims, antiAliasingLevel);
            m_OutputTexture.set_dimensions(viewportDims);
            m_OutputTexture.set_anti_aliasing_level(antiAliasingLevel);
        }

        renderSceneToGBuffers();
        renderLightingPass();
        renderLightCubes();
        graphics::blit_to_screen(m_OutputTexture, viewportRect);
        drawGBufferOverlays(viewportRect);
    }

    void renderSceneToGBuffers()
    {
        m_GBuffer.material.set_texture("uDiffuseMap", m_DiffuseMap);
        m_GBuffer.material.set_texture("uSpecularMap", m_SpecularMap);

        // render scene cubes
        for (Vec3 const& objectPosition : c_ObjectPositions)
        {
            graphics::draw(
                m_CubeMesh,
                {.scale = Vec3{0.5f}, .position = objectPosition},
                m_GBuffer.material,
                m_Camera
            );
        }
        m_Camera.render_to(m_GBuffer.renderTarget);
    }

    void drawGBufferOverlays(Rect const& viewportRect) const
    {
        graphics::blit_to_screen(
            m_GBuffer.albedo,
            Rect{viewportRect.p1, viewportRect.p1 + 200.0f}
        );
        graphics::blit_to_screen(
            m_GBuffer.normal,
            Rect{viewportRect.p1 + Vec2{200.0f, 0.0f}, viewportRect.p1 + Vec2{200.0f, 0.0f} + 200.0f}
        );
        graphics::blit_to_screen(
            m_GBuffer.position,
            Rect{viewportRect.p1 + Vec2{400.0f, 0.0f}, viewportRect.p1 + Vec2{400.0f, 0.0f} + 200.0f}
        );
    }

    void renderLightingPass()
    {
        m_LightPass.material.set_render_texture("uPositionTex", m_GBuffer.position);
        m_LightPass.material.set_render_texture("uNormalTex", m_GBuffer.normal);
        m_LightPass.material.set_render_texture("uAlbedoTex", m_GBuffer.albedo);
        m_LightPass.material.set_vec3_array("uLightPositions", m_LightPositions);
        m_LightPass.material.set_vec3_array("uLightColors", m_LightColors);
        m_LightPass.material.set_float("uLightLinear", 0.7f);
        m_LightPass.material.set_float("uLightQuadratic", 1.8f);
        m_LightPass.material.set_vec3("uViewPos", m_Camera.position());

        graphics::draw(m_QuadMesh, identity<Transform>(), m_LightPass.material, m_Camera);

        m_Camera.render_to(m_OutputTexture);

        m_LightPass.material.clear_render_texture("uPositionTex");
        m_LightPass.material.clear_render_texture("uNormalTex");
        m_LightPass.material.clear_render_texture("uAlbedoTex");
    }

    void renderLightCubes()
    {
        OSC_ASSERT(m_LightPositions.size() == m_LightColors.size());

        for (size_t i = 0; i < m_LightPositions.size(); ++i) {
            m_LightBoxMaterial.set_vec3("uLightColor", m_LightColors[i]);
            graphics::draw(m_CubeMesh, {.scale = Vec3{0.125f}, .position = m_LightPositions[i]}, m_LightBoxMaterial, m_Camera);
        }

        RenderTarget t{
            {
                RenderTargetColorAttachment{
                    m_OutputTexture.upd_color_buffer(),
                    RenderBufferLoadAction::Load,
                    RenderBufferStoreAction::Resolve,
                    Color::clear(),
                },
            },
            RenderTargetDepthAttachment{
                m_GBuffer.albedo.upd_depth_buffer(),
                RenderBufferLoadAction::Load,
                RenderBufferStoreAction::DontCare,
            },
        };
        m_Camera.render_to(t);
    }

    ResourceLoader m_Loader = App::resource_loader();

    // scene state
    std::vector<Vec3> m_LightPositions = GenerateNSceneLightPositions(c_NumLights);
    std::vector<Vec3> m_LightColors = GenerateNSceneLightColors(c_NumLights);
    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f};
    Texture2D m_DiffuseMap = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/container2.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );
    Texture2D m_SpecularMap = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/container2_specular.png"),
        ColorSpace::sRGB,
        ImageLoadingFlags::FlipVertically
    );

    // rendering state
    GBufferRenderingState m_GBuffer{m_Loader};
    LightPassState m_LightPass{m_Loader};

    Material m_LightBoxMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightBox.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedLighting/deferred_shading/LightBox.frag"),
    }};

    RenderTexture m_OutputTexture;
};


// public API

CStringView osc::LOGLDeferredShadingTab::id()
{
    return c_TabStringID;
}

osc::LOGLDeferredShadingTab::LOGLDeferredShadingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLDeferredShadingTab::LOGLDeferredShadingTab(LOGLDeferredShadingTab&&) noexcept = default;
osc::LOGLDeferredShadingTab& osc::LOGLDeferredShadingTab::operator=(LOGLDeferredShadingTab&&) noexcept = default;
osc::LOGLDeferredShadingTab::~LOGLDeferredShadingTab() noexcept = default;

UID osc::LOGLDeferredShadingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLDeferredShadingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLDeferredShadingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLDeferredShadingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLDeferredShadingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLDeferredShadingTab::implOnDraw()
{
    m_Impl->onDraw();
}
