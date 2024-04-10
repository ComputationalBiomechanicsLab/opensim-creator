#include "LOGLPBRDiffuseIrradianceTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>

namespace graphics = osc::graphics;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/DiffuseIrradiance";

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
        cubemapRenderTarget.setDimensionality(TextureDimensionality::Cube);
        cubemapRenderTarget.setColorFormat(RenderTextureFormat::RGBFloat16);

        // create a 90 degree cube cone projection matrix
        Mat4 const projectionMatrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/EquirectangularToCubemap.frag"),
        }};
        material.set_texture("uEquirectangularMap", hdrTexture);
        material.set_mat4_array("uShadowMatrices", CalcCubemapViewProjMatrices(projectionMatrix, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(cubemapRenderTarget);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return cubemapRenderTarget;
    }

    RenderTexture CreateIrradianceCubemap(
        IResourceLoader& rl,
        RenderTexture const& skybox)
    {
        RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.setDimensionality(TextureDimensionality::Cube);
        irradianceCubemap.setColorFormat(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Convolution.frag"),
        }};
        material.set_render_texture("uEnvironmentMap", skybox);
        material.set_mat4_array("uShadowMatrices", CalcCubemapViewProjMatrices(captureProjection, Vec3{}));

        Camera camera;
        graphics::draw(BoxGeometry{2.0f, 2.0f, 2.0f}, identity<Transform>(), material, camera);
        camera.render_to(irradianceCubemap);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return irradianceCubemap;
    }

    Material CreateMaterial(IResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/PBR.frag"),
        }};
        rv.set_float("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRDiffuseIrradianceTab::Impl final : public StandardTabImpl {
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
        draw3DRender();
        drawBackground();
        draw2DUI();
    }

    void draw3DRender()
    {
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());

        m_PBRMaterial.set_vec3("uCameraWorldPos", m_Camera.position());
        m_PBRMaterial.set_vec3_array("uLightPositions", c_LightPositions);
        m_PBRMaterial.set_vec3_array("uLightColors", c_LightRadiances);
        m_PBRMaterial.set_render_texture("uIrradianceMap", m_IrradianceMap);

        drawSpheres();
        drawLights();

        m_Camera.render_to_screen();
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
            graphics::draw(m_SphereMesh, {.scale = Vec3{0.5f}, .position = pos}, m_PBRMaterial, m_Camera);
        }
    }

    void drawBackground()
    {
        m_BackgroundMaterial.set_render_texture("uEnvironmentMap", m_ProjectedMap);
        m_BackgroundMaterial.set_depth_function(DepthFunction::LessOrEqual);  // for skybox depth trick
        graphics::draw(m_CubeMesh, identity<Transform>(), m_BackgroundMaterial, m_Camera);
        m_Camera.set_pixel_rect(ui::GetMainViewportWorkspaceScreenRect());
        m_Camera.set_clear_flags(CameraClearFlags::Nothing);
        m_Camera.render_to_screen();
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

    Material m_BackgroundMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Background.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/diffuse_irradiance/Background.frag"),
    }};

    Mesh m_CubeMesh = BoxGeometry{2.0f, 2.0f, 2.0f};
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    Mesh m_SphereMesh = SphereGeometry{1.0f, 64, 64};
    MouseCapturingCamera m_Camera = CreateCamera();
};


// public API

CStringView osc::LOGLPBRDiffuseIrradianceTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab& osc::LOGLPBRDiffuseIrradianceTab::operator=(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab::~LOGLPBRDiffuseIrradianceTab() noexcept = default;

UID osc::LOGLPBRDiffuseIrradianceTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRDiffuseIrradianceTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRDiffuseIrradianceTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRDiffuseIrradianceTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRDiffuseIrradianceTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRDiffuseIrradianceTab::implOnDraw()
{
    m_Impl->onDraw();
}
