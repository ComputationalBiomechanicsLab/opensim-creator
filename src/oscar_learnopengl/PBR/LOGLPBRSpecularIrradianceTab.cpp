#include "LOGLPBRSpecularIrradianceTab.hpp"

#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <array>
#include <string>
#include <utility>

using namespace osc::literals;
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
using osc::Mesh;
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
        cubemapRenderTarget.setColorFormat(RenderTextureFormat::ARGBFloat16);

        // create a 90 degree cube cone projection matrix
        Mat4 const projectionMatrix = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        // create material that projects all 6 faces onto the output cubemap
        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/EquirectangularToCubemap.frag"),
        }};
        material.setTexture("uEquirectangularMap", hdrTexture);
        material.setMat4Array(
            "uShadowMatrices",
            CalcCubemapViewProjMatrices(projectionMatrix, Vec3{})
        );

        Camera camera;
        osc::Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
        camera.renderTo(cubemapRenderTarget);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return cubemapRenderTarget;
    }

    RenderTexture CreateIrradianceCubemap(RenderTexture const& skybox)
    {
        RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.setDimensionality(TextureDimensionality::Cube);
        irradianceCubemap.setColorFormat(RenderTextureFormat::ARGBFloat16);

        Mat4 const captureProjection = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/IrradianceConvolution.frag"),
        }};
        material.setRenderTexture("uEnvironmentMap", skybox);
        material.setMat4Array(
            "uShadowMatrices",
            CalcCubemapViewProjMatrices(captureProjection, Vec3{})
        );

        Camera camera;
        osc::Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
        camera.renderTo(irradianceCubemap);

        // TODO: some way of copying it into an `Cubemap` would make sense
        return irradianceCubemap;
    }

    Cubemap CreatePreFilteredEnvironmentMap(
        RenderTexture const& environmentMap)
    {
        int constexpr levelZeroWidth = 128;
        static_assert(osc::popcount(static_cast<unsigned>(levelZeroWidth)) == 1);

        RenderTexture captureRT{{levelZeroWidth, levelZeroWidth}};
        captureRT.setDimensionality(TextureDimensionality::Cube);
        captureRT.setColorFormat(RenderTextureFormat::ARGBFloat16);

        Mat4 const captureProjection = Perspective(90_deg, 1.0f, 0.1f, 10.0f);

        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.geom"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Prefilter.frag"),
        }};
        material.setRenderTexture("uEnvironmentMap", environmentMap);
        material.setMat4Array("uShadowMatrices", CalcCubemapViewProjMatrices(captureProjection, Vec3{}));

        Camera camera;

        Cubemap rv{levelZeroWidth, TextureFormat::RGBAFloat};
        rv.setWrapMode(TextureWrapMode::Clamp);
        // TODO: ensure GL_TEXTURE_MIN_FILTER is GL_LINEAR_MIPMAP_LINEAR
        // TODO: ensure GL_TEXTURE_MAG_FILTER is GL_LINEAR

        size_t const maxMipmapLevel = static_cast<size_t>(std::max(
            0,
            osc::bit_width(static_cast<size_t>(levelZeroWidth)) - 1
        ));
        static_assert(maxMipmapLevel == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= maxMipmapLevel; ++mip) {
            size_t const mipWidth = levelZeroWidth >> mip;
            captureRT.setDimensions({static_cast<int>(mipWidth), static_cast<int>(mipWidth)});

            float const roughness = static_cast<float>(mip)/static_cast<float>(maxMipmapLevel);
            material.setFloat("uRoughness", roughness);

            osc::Graphics::DrawMesh(GenerateCubeMesh(), Identity<Transform>(), material, camera);
            camera.renderTo(captureRT);
            osc::Graphics::CopyTexture(captureRT, rv, mip);
        }

        return rv;
    }

    Texture2D Create2DBRDFLookup()
    {
        RenderTexture renderTex{{512, 512}};
        renderTex.setColorFormat(RenderTextureFormat::ARGBFloat16);  // TODO RG16F in LearnOpenGL

        Material material{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/BRDF.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/BRDF.frag"),
        }};

        Mesh quad = GenerateTexturedQuadMesh();

        // TODO: Graphics::Blit with material
        Camera camera;
        camera.setProjectionMatrixOverride(Identity<Mat4>());
        camera.setViewMatrixOverride(Identity<Mat4>());

        osc::Graphics::DrawMesh(quad, Identity<Transform>(), material, camera);
        camera.renderTo(renderTex);

        Texture2D rv{
            {512, 512},
            TextureFormat::RGBFloat,  // TODO: RG16F in LearnOpenGL
            ColorSpace::Linear,
            TextureWrapMode::Clamp,
            TextureFilterMode::Linear,
        };
        osc::Graphics::CopyTexture(renderTex, rv);
        return rv;
    }

    Material CreateMaterial()
    {
        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/PBR.vert"),
            App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/PBR.frag"),
        }};
        rv.setFloat("uAO", 1.0f);
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
        draw2DUI();
        m_PerfPanel.onDraw();
    }

    void draw3DRender()
    {
        m_PBRMaterial.setVec3("uCameraWorldPos", m_Camera.getPosition());
        m_PBRMaterial.setVec3Array("uLightPositions", c_LightPositions);
        m_PBRMaterial.setVec3Array("uLightColors", c_LightRadiances);
        m_PBRMaterial.setRenderTexture("uIrradianceMap", m_IrradianceMap);
        m_PBRMaterial.setCubemap("uPrefilterMap", m_PrefilterMap);
        m_PBRMaterial.setFloat("uMaxReflectionLOD", static_cast<float>(osc::bit_width(static_cast<size_t>(m_PrefilterMap.getWidth()) - 1)));
        m_PBRMaterial.setTexture("uBRDFLut", m_BRDFLookup);

        drawSpheres();
        drawLights();

        m_Camera.renderTo(m_OutputRender);
    }

    void drawSpheres()
    {
        m_PBRMaterial.setVec3("uAlbedoColor", {0.5f, 0.0f, 0.0f});

        for (int row = 0; row < c_NumRows; ++row) {
            m_PBRMaterial.setFloat("uMetallicity", static_cast<float>(row) / static_cast<float>(c_NumRows));

            for (int col = 0; col < c_NumCols; ++col) {
                float const normalizedCol = static_cast<float>(col) / static_cast<float>(c_NumCols);
                m_PBRMaterial.setFloat("uRoughness", Clamp(normalizedCol, 0.005f, 1.0f));

                float const x = (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing;
                float const y = (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing;

                Graphics::DrawMesh(m_SphereMesh, {.position = {x, y, 0.0f}}, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        m_PBRMaterial.setVec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

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

    void draw2DUI()
    {
        if (ImGui::Begin("Controls")) {
            float ao = m_PBRMaterial.getFloat("uAO").value_or(1.0f);
            if (ImGui::SliderFloat("ao", &ao, 0.0f, 1.0f)) {
                m_PBRMaterial.setFloat("uAO", ao);
            }
        }
        ImGui::End();
    }

    Texture2D m_Texture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap();
    RenderTexture m_IrradianceMap = CreateIrradianceCubemap(m_ProjectedMap);
    Cubemap m_PrefilterMap = CreatePreFilteredEnvironmentMap(m_ProjectedMap);
    Texture2D m_BRDFLookup = Create2DBRDFLookup();
    RenderTexture m_OutputRender{{1, 1}};

    Material m_BackgroundMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Skybox.vert"),
        App::slurp("oscar_learnopengl/shaders/PBR/ibl_specular/Skybox.frag"),
    }};

    Mesh m_CubeMesh = GenerateCubeMesh();
    Material m_PBRMaterial = CreateMaterial();
    Mesh m_SphereMesh = GenerateUVSphereMesh(64, 64);

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

osc::UID osc::LOGLPBRSpecularIrradianceTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLPBRSpecularIrradianceTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRSpecularIrradianceTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRSpecularIrradianceTab::implOnDraw()
{
    m_Impl->onDraw();
}
