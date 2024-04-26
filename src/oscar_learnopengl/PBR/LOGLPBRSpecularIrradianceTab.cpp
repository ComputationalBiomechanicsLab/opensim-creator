#include "LOGLPBRSpecularIrradianceTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <bit>
#include <utility>

namespace graphics = osc::graphics;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/SpecularIrradiance";

    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_LightPositions.size()> c_LightRadiances = std::to_array<Vec3>({
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_NumRows = 7;
    constexpr int c_NumCols = 7;
    constexpr float c_CellSpacing = 2.5f;

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.set_position({0.0f, 0.0f, 20.0f});
        rv.set_vertical_fov(45_deg);
        rv.set_near_clipping_plane(0.1f);
        rv.set_far_clipping_plane(100.0f);
        rv.set_background_color({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture LoadEquirectangularHDRTextureIntoCubemap(
        IResourceLoader& rl)
    {
        Texture2D hdrTexture = load_texture2D_from_image(
            rl.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear,
            ImageLoadingFlags::FlipVertically
        );
        hdrTexture.set_wrap_mode(TextureWrapMode::Clamp);
        hdrTexture.set_filter_mode(TextureFilterMode::Linear);

        RenderTexture cubemapRenderTarget{{512, 512}};
        cubemapRenderTarget.set_dimensionality(TextureDimensionality::Cube);
        cubemapRenderTarget.set_color_format(RenderTextureFormat::RGBFloat16);

        // create a 90 degree cube cone projection matrix
        Mat4 const projectionMatrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.frag"),
        }};
        material.set_texture("uEquirectangularMap", hdrTexture);
        material.set_mat4_array(
            "uShadowMatrices",
            calc_cubemap_view_proj_matrices(projectionMatrix, Vec3{})
        );

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(cubemapRenderTarget);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemapRenderTarget;
    }

    RenderTexture CreateIrradianceCubemap(
        IResourceLoader& rl,
        RenderTexture const& skybox)
    {
        RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.set_dimensionality(TextureDimensionality::Cube);
        irradianceCubemap.set_color_format(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.frag"),
        }};
        material.set_render_texture("uEnvironmentMap", skybox);
        material.set_mat4_array(
            "uShadowMatrices",
            calc_cubemap_view_proj_matrices(captureProjection, Vec3{})
        );

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(irradianceCubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradianceCubemap;
    }

    Cubemap CreatePreFilteredEnvironmentMap(
        IResourceLoader& rl,
        RenderTexture const& environmentMap)
    {
        int constexpr levelZeroWidth = 128;
        static_assert(std::popcount(static_cast<unsigned>(levelZeroWidth)) == 1);

        RenderTexture captureRT{{levelZeroWidth, levelZeroWidth}};
        captureRT.set_dimensionality(TextureDimensionality::Cube);
        captureRT.set_color_format(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.frag"),
        }};
        material.set_render_texture("uEnvironmentMap", environmentMap);
        material.set_mat4_array("uShadowMatrices", calc_cubemap_view_proj_matrices(captureProjection, Vec3{}));

        Camera camera;

        Cubemap rv{levelZeroWidth, TextureFormat::RGBFloat};  // TODO: add support for TextureFormat:::RGFloat16
        rv.set_wrap_mode(TextureWrapMode::Clamp);
        rv.set_filter_mode(TextureFilterMode::Mipmap);

        auto const maxMipmapLevel = static_cast<size_t>(max(
            0,
            static_cast<int>(std::bit_width(static_cast<size_t>(levelZeroWidth))) - 1
        ));
        static_assert(maxMipmapLevel == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= maxMipmapLevel; ++mip) {
            size_t const mipWidth = levelZeroWidth >> mip;
            captureRT.set_dimensions({static_cast<int>(mipWidth), static_cast<int>(mipWidth)});

            float const roughness = static_cast<float>(mip)/static_cast<float>(maxMipmapLevel);
            material.set_float("uRoughness", roughness);

            graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
            camera.render_to(captureRT);
            graphics::copy_texture(captureRT, rv, mip);
        }

        return rv;
    }

    Texture2D Create2DBRDFLookup(
        IResourceLoader& rl)
    {
        RenderTexture renderTex{{512, 512}};
        renderTex.set_color_format(RenderTextureFormat::RGFloat16);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/BRDF.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/BRDF.frag"),
        }};

        // TODO: graphics::blit with material
        Camera camera;
        camera.set_projection_matrix_override(identity<Mat4>());
        camera.set_view_matrix_override(identity<Mat4>());

        graphics::draw(PlaneGeometry{2.0f, 2.0f, 1, 1}, identity<Transform>(), material, camera);
        camera.render_to(renderTex);

        Texture2D rv{
            {512, 512},
            TextureFormat::RGFloat,  // TODO: add support for TextureFormat:::RGFloat16
            ColorSpace::Linear,
            TextureWrapMode::Clamp,
            TextureFilterMode::Linear,
        };
        graphics::copy_texture(renderTex, rv);
        return rv;
    }

    Material CreateMaterial(
        IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/PBR.frag"),
        }};
        rv.set_float("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRSpecularIrradianceTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        App::upd().make_main_loop_polling();
        m_Camera.on_mount();
    }

    void implOnUnmount() final
    {
        m_Camera.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.on_event(e);
    }

    void implOnDraw() final
    {
        Rect const outputRect = ui::GetMainViewportWorkspaceScreenRect();
        m_OutputRender.set_dimensions(dimensions_of(outputRect));
        m_OutputRender.set_anti_aliasing_level(App::get().anti_aliasing_level());

        m_Camera.on_draw();
        draw3DRender();
        drawBackground();
        graphics::blit_to_screen(m_OutputRender, outputRect);
        draw2DUI();
        m_PerfPanel.on_draw();
    }

    void draw3DRender()
    {
        m_PBRMaterial.set_vec3("uCameraWorldPos", m_Camera.position());
        m_PBRMaterial.set_vec3_array("uLightPositions", c_LightPositions);
        m_PBRMaterial.set_vec3_array("uLightColors", c_LightRadiances);
        m_PBRMaterial.set_render_texture("uIrradianceMap", m_IrradianceMap);
        m_PBRMaterial.set_cubemap("uPrefilterMap", m_PrefilterMap);
        m_PBRMaterial.set_float("uMaxReflectionLOD", static_cast<float>(std::bit_width(static_cast<size_t>(m_PrefilterMap.width()) - 1)));
        m_PBRMaterial.set_texture("uBRDFLut", m_BRDFLookup);

        drawSpheres();
        drawLights();

        m_Camera.render_to(m_OutputRender);
    }

    void drawSpheres()
    {
        m_PBRMaterial.set_vec3("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_NumRows; ++row) {
            m_PBRMaterial.set_float("uMetallicity", static_cast<float>(row) / static_cast<float>(c_NumRows));

            for (int col = 0; col < c_NumCols; ++col) {
                float const normalizedCol = static_cast<float>(col) / static_cast<float>(c_NumCols);
                m_PBRMaterial.set_float("uRoughness", clamp(normalizedCol, 0.005f, 1.0f));

                float const x = (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing;
                float const y = (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing;

                graphics::draw(m_SphereMesh, {.position = {x, y, 0.0f}}, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        m_PBRMaterial.set_vec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (Vec3 const& pos : c_LightPositions) {
            graphics::draw(
                m_SphereMesh,
                {.scale = Vec3{0.5f}, .position = pos},
                m_PBRMaterial,
                m_Camera
            );
        }
    }

    void drawBackground()
    {
        m_BackgroundMaterial.set_render_texture("uEnvironmentMap", m_ProjectedMap);
        m_BackgroundMaterial.set_depth_function(DepthFunction::LessOrEqual);  // for skybox depth trick
        graphics::draw(m_CubeMesh, identity<Transform>(), m_BackgroundMaterial, m_Camera);
        m_Camera.set_clear_flags(CameraClearFlags::Nothing);
        m_Camera.render_to(m_OutputRender);
        m_Camera.set_clear_flags(CameraClearFlags::Default);
    }

    void draw2DUI()
    {
        if (ui::Begin("Controls")) {
            float ao = m_PBRMaterial.get_float("uAO").value_or(1.0f);
            if (ui::SliderFloat("ao", &ao, 0.0f, 1.0f)) {
                m_PBRMaterial.set_float("uAO", ao);
            }
        }
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();

    Texture2D m_Texture = load_texture2D_from_image(
        m_Loader.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap(m_Loader);
    RenderTexture m_IrradianceMap = CreateIrradianceCubemap(m_Loader, m_ProjectedMap);
    Cubemap m_PrefilterMap = CreatePreFilteredEnvironmentMap(m_Loader, m_ProjectedMap);
    Texture2D m_BRDFLookup = Create2DBRDFLookup(m_Loader);
    RenderTexture m_OutputRender{{1, 1}};

    Material m_BackgroundMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Skybox.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Skybox.frag"),
    }};

    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    Mesh m_SphereMesh = SphereGeometry{1.0f, 64, 64};

    MouseCapturingCamera m_Camera = CreateCamera();

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPBRSpecularIrradianceTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRSpecularIrradianceTab::LOGLPBRSpecularIrradianceTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRSpecularIrradianceTab::LOGLPBRSpecularIrradianceTab(LOGLPBRSpecularIrradianceTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTab& osc::LOGLPBRSpecularIrradianceTab::operator=(LOGLPBRSpecularIrradianceTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTab::~LOGLPBRSpecularIrradianceTab() noexcept = default;

UID osc::LOGLPBRSpecularIrradianceTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRSpecularIrradianceTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnMount()
{
    m_Impl->on_mount();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnUnmount()
{
    m_Impl->on_unmount();
}

bool osc::LOGLPBRSpecularIrradianceTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRSpecularIrradianceTab::implOnDraw()
{
    m_Impl->onDraw();
}
