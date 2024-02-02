#include "LOGLPBRSpecularIrradianceTexturedTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <SDL_events.h>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <array>
#include <utility>

using namespace osc::literals;
namespace Graphics = osc::Graphics;
namespace cpp20 = osc::cpp20;
using osc::App;
using osc::CalcCubemapViewProjMatrices;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::Cubemap;
using osc::GenerateCubeMesh;
using osc::GenerateTexturedQuadMesh;
using osc::Identity;
using osc::ImageLoadingFlags;
using osc::LoadTexture2DFromImage;
using osc::Mat4;
using osc::Material;
using osc::MouseCapturingCamera;
using osc::Perspective;
using osc::RenderTexture;
using osc::RenderTextureFormat;
using osc::Shader;
using osc::Texture2D;
using osc::TextureDimensionality;
using osc::TextureFilterMode;
using osc::TextureFormat;
using osc::TextureWrapMode;
using osc::Transform;
using osc::Vec3;

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
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    RenderTexture LoadEquirectangularHDRTextureIntoCubemap()
    {
        Texture2D hdrTexture = LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
            ColorSpace::Linear,
            ImageLoadingFlags::FlipVertically
        );
        hdrTexture.setWrapMode(TextureWrapMode::Clamp);
        hdrTexture.setFilterMode(TextureFilterMode::Linear);

        RenderTexture cubemapRenderTarget{{512, 512}};
        cubemapRenderTarget.setDimensionality(TextureDimensionality::Cube);
        cubemapRenderTarget.setColorFormat(RenderTextureFormat::RGBFloat16);

        // create a 90 degree cube cone projection matrix
        Mat4 const projectionMatrix = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/EquirectangularToCubemap.frag"),
        }};
        material.setTexture("uEquirectangularMap", hdrTexture);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(projectionMatrix, Vec3{}));

        Camera camera;
        Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
        camera.renderTo(cubemapRenderTarget);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemapRenderTarget;
    }

    RenderTexture CreateIrradianceCubemap(RenderTexture const& skybox)
    {
        RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.setDimensionality(TextureDimensionality::Cube);
        irradianceCubemap.setColorFormat(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/IrradianceConvolution.frag"),
        }};
        material.setRenderTexture("uEnvironmentMap", skybox);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(captureProjection, Vec3{}));

        Camera camera;
        Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
        camera.renderTo(irradianceCubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradianceCubemap;
    }

    Cubemap CreatePreFilteredEnvironmentMap(
        RenderTexture const& environmentMap)
    {
        int constexpr levelZeroWidth = 128;
        static_assert(cpp20::popcount(static_cast<unsigned>(levelZeroWidth)) == 1);

        RenderTexture captureRT{{levelZeroWidth, levelZeroWidth}};
        captureRT.setDimensionality(TextureDimensionality::Cube);
        captureRT.setColorFormat(RenderTextureFormat::RGBFloat16);

        Mat4 const captureProjection = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Prefilter.frag"),
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

            Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
            camera.renderTo(captureRT);
            Graphics::CopyTexture(captureRT, rv, mip);
        }

        return rv;
    }

    Texture2D Create2DBRDFLookup()
    {
        // TODO: Graphics::Blit with material
        Camera camera;
        camera.setProjectionMatrixOverride(Identity<Mat4>());
        camera.setViewMatrixOverride(Identity<Mat4>());

        Graphics::DrawMesh(
            GenerateTexturedQuadMesh(),
            Identity<Transform>(),
            Material{Shader{
                App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.vert"),
                App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/BRDF.frag"),
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

    Material CreateMaterial()
    {
        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/PBR.frag"),
        }};
        rv.setFloat("uAO", 1.0f);
        return rv;
    }

    struct IBLSpecularObjectTextures final {
        explicit IBLSpecularObjectTextures(std::filesystem::path dir_) :
            dir{std::move(dir_)}
        {}

        std::filesystem::path dir;

        Texture2D albedoMap = LoadTexture2DFromImage(dir / "albedo.png", ColorSpace::sRGB);
        Texture2D normalMap = LoadTexture2DFromImage(dir / "normal.png", ColorSpace::Linear);
        Texture2D metallicMap = LoadTexture2DFromImage(dir / "metallic.png", ColorSpace::Linear);
        Texture2D roughnessMap = LoadTexture2DFromImage(dir / "roughness.png", ColorSpace::Linear);
        Texture2D aoMap = LoadTexture2DFromImage(dir / "ao.png", ColorSpace::Linear);
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
        Rect const outputRect = GetMainViewportWorkspaceScreenRect();
        m_OutputRender.setDimensions(Dimensions(outputRect));
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

        Graphics::DrawMesh(m_CubeMesh, Identity<Transform>(), m_BackgroundMaterial, m_Camera);

        m_Camera.setClearFlags(CameraClearFlags::Nothing);
        m_Camera.renderTo(m_OutputRender);
        m_Camera.setClearFlags(CameraClearFlags::Default);
    }

    Texture2D m_Texture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    std::array<IBLSpecularObjectTextures, 5> m_ObjectTextures = std::to_array<IBLSpecularObjectTextures>({
        IBLSpecularObjectTextures{App::resource("oscar_learnopengl/textures/pbr/rusted_iron")},
        IBLSpecularObjectTextures{App::resource("oscar_learnopengl/textures/pbr/gold")},
        IBLSpecularObjectTextures{App::resource("oscar_learnopengl/textures/pbr/grass")},
        IBLSpecularObjectTextures{App::resource("oscar_learnopengl/textures/pbr/plastic")},
        IBLSpecularObjectTextures{App::resource("oscar_learnopengl/textures/pbr/wall")},
    });

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap();
    RenderTexture m_IrradianceMap = CreateIrradianceCubemap(m_ProjectedMap);
    Cubemap m_PrefilterMap = CreatePreFilteredEnvironmentMap(m_ProjectedMap);
    Texture2D m_BRDFLookup = Create2DBRDFLookup();
    RenderTexture m_OutputRender{{1, 1}};

    Material m_BackgroundMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.vert"),
        App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular_textured/Skybox.frag"),
    }};

    Mesh m_CubeMesh = GenerateCubeMesh();
    Material m_PBRMaterial = CreateMaterial();
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

osc::UID osc::LOGLPBRSpecularIrradianceTexturedTab::implGetID() const
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
