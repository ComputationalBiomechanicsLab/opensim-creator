#include "LOGLPBRSpecularIrradianceTexturedTab.h"

#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <SDL_events.h>
#include <oscar/oscar.h>

#include <array>
#include <utility>

namespace Graphics = osc::Graphics;
namespace cpp20 = osc::cpp20;
using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/PBR/SpecularIrradianceTextured";

    constexpr auto c_LightPositions = std::to_array<Vec3>({
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<Vec3, c_LightPositions.size()> c_LightRadiances = std::to_array<Vec3>({
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
        {150.0f, 150.0f, 150.0f},
    });

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 20.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture LoadEquirectangularHDRTextureIntoCubemap(ResourceLoader& rl)
    {
        Texture2D hdrTexture = LoadTexture2DFromImage(
            rl.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear,
            ImageLoadingFlags::FlipVertically
        );
        hdrTexture.setWrapMode(TextureWrapMode::Clamp);
        hdrTexture.setFilterMode(TextureFilterMode::Linear);

        RenderTexture cubemapRenderTarget{{512, 512}};
        cubemapRenderTarget.setDimensionality(TextureDimensionality::Cube);
        cubemapRenderTarget.setColorFormat(RenderTextureFormat::RGBFloat16);

        // create a 90 degree cube cone projection matrix
        Mat4 const projectionMatrix = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.frag"),
        }};
        material.setTexture("uEquirectangularMap", hdrTexture);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(projectionMatrix, Vec3{}));

        Camera camera;
        Graphics::DrawMesh(GenerateCubeMesh(), identity<Transform>(), material, camera);
        camera.renderTo(cubemapRenderTarget);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemapRenderTarget;
    }

    RenderTexture CreateIrradianceCubemap(
        ResourceLoader& rl,
        RenderTexture const& skybox)
    {
        RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.setDimensionality(TextureDimensionality::Cube);
        irradianceCubemap.setColorFormat(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.frag"),
        }};
        material.setRenderTexture("uEnvironmentMap", skybox);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(captureProjection, Vec3{}));

        Camera camera;
        Graphics::DrawMesh(GenerateCubeMesh(), identity<Transform>(), material, camera);
        camera.renderTo(irradianceCubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradianceCubemap;
    }

    Cubemap CreatePreFilteredEnvironmentMap(
        ResourceLoader& rl,
        RenderTexture const& environmentMap)
    {
        int constexpr levelZeroWidth = 128;
        static_assert(cpp20::popcount(static_cast<unsigned>(levelZeroWidth)) == 1);

        RenderTexture captureRT{{levelZeroWidth, levelZeroWidth}};
        captureRT.setDimensionality(TextureDimensionality::Cube);
        captureRT.setColorFormat(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.geom"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.frag"),
        }};
        material.setRenderTexture("uEnvironmentMap", environmentMap);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(captureProjection, Vec3{}));

        Camera camera;

        Cubemap rv{levelZeroWidth, TextureFormat::RGBAFloat};
        rv.setWrapMode(TextureWrapMode::Clamp);
        rv.setFilterMode(TextureFilterMode::Mipmap);

        constexpr size_t maxMipmapLevel = static_cast<size_t>(std::max(
            0,
            cpp20::bit_width(static_cast<size_t>(levelZeroWidth)) - 1
        ));
        static_assert(maxMipmapLevel == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= maxMipmapLevel; ++mip) {
            size_t const mipWidth = levelZeroWidth >> mip;
            captureRT.setDimensions({static_cast<int>(mipWidth), static_cast<int>(mipWidth)});

            float const roughness = static_cast<float>(mip)/static_cast<float>(maxMipmapLevel);
            material.setFloat("uRoughness", roughness);

            Graphics::DrawMesh(GenerateCubeMesh(), identity<Transform>(), material, camera);
            camera.renderTo(captureRT);
            Graphics::CopyTexture(captureRT, rv, mip);
        }

        return rv;
    }

    Texture2D Create2DBRDFLookup(
        ResourceLoader& rl)
    {
        // TODO: Graphics::Blit with material
        Camera camera;
        camera.setProjectionMatrixOverride(identity<Mat4>());
        camera.setViewMatrixOverride(identity<Mat4>());

        Graphics::DrawMesh(
            GeneratePlaneMesh2(2.0f, 2.0f, 1, 1),
            identity<Transform>(),
            Material{Shader{
                rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.vert"),
                rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.frag"),
            }},
            camera
        );

        RenderTexture renderTex{{512, 512}};
        renderTex.setColorFormat(RenderTextureFormat::RGFloat16);
        camera.renderTo(renderTex);

        Texture2D rv{
            {512, 512},
            TextureFormat::RGFloat,  // TODO: add support for TextureFormat:::RGFloat16
            ColorSpace::Linear,
            TextureWrapMode::Clamp,
            TextureFilterMode::Linear,
        };
        Graphics::CopyTexture(renderTex, rv);
        return rv;
    }

    Material CreateMaterial(ResourceLoader& rl)
    {
        Material rv{Shader{
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.vert"),
            rl.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.frag"),
        }};
        rv.setFloat("uAO", 1.0f);
        return rv;
    }

    struct IBLSpecularObjectTextures final {
        explicit IBLSpecularObjectTextures(ResourceLoader loader_) :
            albedoMap{LoadTexture2DFromImage(loader_.open("albedo.png"), ColorSpace::sRGB)},
            normalMap{LoadTexture2DFromImage(loader_.open("normal.png"), ColorSpace::Linear)},
            metallicMap{LoadTexture2DFromImage(loader_.open("metallic.png"), ColorSpace::Linear)},
            roughnessMap{LoadTexture2DFromImage(loader_.open("roughness.png"), ColorSpace::Linear)},
            aoMap{LoadTexture2DFromImage(loader_.open("ao.png"), ColorSpace::Linear)}
        {}

        Texture2D albedoMap;
        Texture2D normalMap;
        Texture2D metallicMap;
        Texture2D roughnessMap;
        Texture2D aoMap;
    };
}

class osc::LOGLPBRSpecularIrradianceTexturedTab::Impl final : public StandardTabImpl {
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
        Rect const outputRect = ui::GetMainViewportWorkspaceScreenRect();
        m_OutputRender.setDimensions(dimensions(outputRect));
        m_OutputRender.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());

        m_Camera.onDraw();
        draw3DRender();
        drawBackground();
        Graphics::BlitToScreen(m_OutputRender, outputRect);
        m_PerfPanel.onDraw();
    }

    void draw3DRender()
    {
        setCommonMaterialProps();
        drawSpheres();
        drawLights();

        m_Camera.renderTo(m_OutputRender);
    }

    void setCommonMaterialProps()
    {
        m_PBRMaterial.setVec3("uCameraWorldPos", m_Camera.getPosition());
        m_PBRMaterial.setVec3Array("uLightPositions", c_LightPositions);
        m_PBRMaterial.setVec3Array("uLightColors", c_LightRadiances);
        m_PBRMaterial.setRenderTexture("uIrradianceMap", m_IrradianceMap);
        m_PBRMaterial.setCubemap("uPrefilterMap", m_PrefilterMap);
        m_PBRMaterial.setFloat("uMaxReflectionLOD", static_cast<float>(cpp20::bit_width(static_cast<size_t>(m_PrefilterMap.getWidth()) - 1)));
        m_PBRMaterial.setTexture("uBRDFLut", m_BRDFLookup);
    }

    void setMaterialMaps(Material& mat, IBLSpecularObjectTextures const& ts)
    {
        mat.setTexture("uAlbedoMap", ts.albedoMap);
        mat.setTexture("uNormalMap", ts.normalMap);
        mat.setTexture("uMetallicMap", ts.metallicMap);
        mat.setTexture("uRoughnessMap", ts.roughnessMap);
        mat.setTexture("uAOMap", ts.aoMap);
    }

    void drawSpheres()
    {
        Vec3 pos = {-5.0f, 0.0f, 2.0f};
        for (IBLSpecularObjectTextures const& t : m_ObjectTextures) {
            setMaterialMaps(m_PBRMaterial, t);
            Graphics::DrawMesh(m_SphereMesh, {.position = pos}, m_PBRMaterial, m_Camera);
            pos.x += 2.0f;
        }
    }

    void drawLights()
    {
        for (Vec3 const& pos : c_LightPositions) {
            Graphics::DrawMesh(
                m_SphereMesh,
                {.scale = Vec3{0.5f}, .position = pos},
                m_PBRMaterial,
                m_Camera
            );
        }
    }

    void drawBackground()
    {
        m_BackgroundMaterial.setRenderTexture("uEnvironmentMap", m_ProjectedMap);
        m_BackgroundMaterial.setDepthFunction(DepthFunction::LessOrEqual);  // for skybox depth trick

        Graphics::DrawMesh(m_CubeMesh, identity<Transform>(), m_BackgroundMaterial, m_Camera);

        m_Camera.setClearFlags(CameraClearFlags::Nothing);
        m_Camera.renderTo(m_OutputRender);
        m_Camera.setClearFlags(CameraClearFlags::Default);
    }

    ResourceLoader m_Loader = App::resource_loader();

    Texture2D m_Texture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    std::array<IBLSpecularObjectTextures, 5> m_ObjectTextures = std::to_array<IBLSpecularObjectTextures>({
        IBLSpecularObjectTextures{m_Loader.withPrefix("oscar_learnopengl/textures/pbr/rusted_iron")},
        IBLSpecularObjectTextures{m_Loader.withPrefix("oscar_learnopengl/textures/pbr/gold")},
        IBLSpecularObjectTextures{m_Loader.withPrefix("oscar_learnopengl/textures/pbr/grass")},
        IBLSpecularObjectTextures{m_Loader.withPrefix("oscar_learnopengl/textures/pbr/plastic")},
        IBLSpecularObjectTextures{m_Loader.withPrefix("oscar_learnopengl/textures/pbr/wall")},
    });

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap(m_Loader);
    RenderTexture m_IrradianceMap = CreateIrradianceCubemap(m_Loader, m_ProjectedMap);
    Cubemap m_PrefilterMap = CreatePreFilteredEnvironmentMap(m_Loader, m_ProjectedMap);
    Texture2D m_BRDFLookup = Create2DBRDFLookup(m_Loader);
    RenderTexture m_OutputRender{{1, 1}};

    Material m_BackgroundMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.frag"),
    }};

    Mesh m_CubeMesh = GenerateCubeMesh();
    Material m_PBRMaterial = CreateMaterial(m_Loader);
    Mesh m_SphereMesh = GenerateUVSphereMesh(64, 64);

    MouseCapturingCamera m_Camera = CreateCamera();

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

CStringView osc::LOGLPBRSpecularIrradianceTexturedTab::id()
{
    return c_TabStringID;
}

osc::LOGLPBRSpecularIrradianceTexturedTab::LOGLPBRSpecularIrradianceTexturedTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRSpecularIrradianceTexturedTab::LOGLPBRSpecularIrradianceTexturedTab(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTexturedTab& osc::LOGLPBRSpecularIrradianceTexturedTab::operator=(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept = default;
osc::LOGLPBRSpecularIrradianceTexturedTab::~LOGLPBRSpecularIrradianceTexturedTab() noexcept = default;

UID osc::LOGLPBRSpecularIrradianceTexturedTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRSpecularIrradianceTexturedTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRSpecularIrradianceTexturedTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRSpecularIrradianceTexturedTab::implOnDraw()
{
    m_Impl->onDraw();
}
