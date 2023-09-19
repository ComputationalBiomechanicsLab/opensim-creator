#include "LOGLPBRSpecularIrradianceTab.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <IconsFontAwesome5.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MeshGen.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <SDL_events.h>

#include <array>
#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/SpecularIrradiance";

    constexpr auto c_LightPositions = osc::to_array<glm::vec3>(
    {
        {-10.0f,  10.0f, 10.0f},
        { 10.0f,  10.0f, 10.0f},
        {-10.0f, -10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f},
    });

    constexpr std::array<glm::vec3, c_LightPositions.size()> c_LightRadiances = osc::to_array<glm::vec3>(
    {
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f},
    });

    constexpr int c_NumRows = 7;
    constexpr int c_NumCols = 7;
    constexpr float c_CellSpacing = 2.5f;

    osc::Camera CreateCamera()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    osc::RenderTexture LoadEquirectangularHDRTextureIntoCubemap()
    {
        int constexpr renderWidth = 512;

        osc::Texture2D hdrTexture = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/hdr/newport_loft.hdr"),
            osc::ColorSpace::Linear,
            osc::ImageLoadingFlags::FlipVertically
        );
        hdrTexture.setWrapMode(osc::TextureWrapMode::Clamp);
        hdrTexture.setFilterMode(osc::TextureFilterMode::Linear);

        osc::RenderTexture cubemapRenderTarget{{renderWidth, renderWidth}};
        cubemapRenderTarget.setDimensionality(osc::TextureDimensionality::Cube);
        cubemapRenderTarget.setColorFormat(osc::RenderTextureFormat::ARGBFloat16);

        // create a 90 degree cube cone projection matrix
        glm::mat4 const projectionMatrix = glm::perspective(
            glm::radians(90.0f),
            1.0f,
            0.1f,
            10.0f
        );

        // create material that projects all 6 faces onto the output cubemap
        osc::Material material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/EquirectangularToCubemap.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/EquirectangularToCubemap.geom"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/EquirectangularToCubemap.frag"),
            }
        };
        material.setTexture("uEquirectangularMap", hdrTexture);
        material.setMat4Array(
            "uShadowMatrices",
            osc::CalcCubemapViewProjMatrices(projectionMatrix, glm::vec3{})
        );

        osc::Camera camera;
        osc::Graphics::DrawMesh(osc::GenCube(), osc::Transform{}, material, camera);
        camera.renderTo(cubemapRenderTarget);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return cubemapRenderTarget;
    }

    osc::RenderTexture CreateIrradianceCubemap(osc::RenderTexture const& skybox)
    {
        osc::RenderTexture irradianceCubemap{{32, 32}};
        irradianceCubemap.setDimensionality(osc::TextureDimensionality::Cube);
        irradianceCubemap.setColorFormat(osc::RenderTextureFormat::ARGBFloat16);

        glm::mat4 const captureProjection = glm::perspective(
            glm::radians(90.0f),
            1.0f,
            0.1f,
            10.0f
        );

        osc::Material material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/IrradianceConvolution.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/IrradianceConvolution.geom"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/IrradianceConvolution.frag"),
            },
        };
        material.setRenderTexture(
            "uEnvironmentMap",
            skybox
        );
        material.setMat4Array(
            "uShadowMatrices",
            osc::CalcCubemapViewProjMatrices(captureProjection, glm::vec3{})
        );

        osc::Camera camera;
        osc::Graphics::DrawMesh(osc::GenCube(), osc::Transform{}, material, camera);
        camera.renderTo(irradianceCubemap);

        // TODO: some way of copying it into an `osc::Cubemap` would make sense
        return irradianceCubemap;
    }

    osc::Cubemap CreatePreFilteredEnvironmentMap(
        osc::RenderTexture const& environmentMap)
    {
        int constexpr levelZeroWidth = 128;
        static_assert(osc::popcount(static_cast<unsigned>(levelZeroWidth)) == 1);

        osc::RenderTexture captureRT{{levelZeroWidth, levelZeroWidth}};
        captureRT.setDimensionality(osc::TextureDimensionality::Cube);
        captureRT.setColorFormat(osc::RenderTextureFormat::ARGBFloat16);

        glm::mat4 const captureProjection = glm::perspective(
            glm::radians(90.0f),
            1.0f,
            0.1f,
            10.0f
        );

        osc::Material material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/Prefilter.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/Prefilter.geom"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/Prefilter.frag"),
            },
        };
        material.setRenderTexture("uEnvironmentMap", environmentMap);
        material.setMat4Array("uShadowMatrices", osc::CalcCubemapViewProjMatrices(captureProjection, glm::vec3{}));

        osc::Camera camera;

        osc::Cubemap rv{levelZeroWidth, osc::TextureFormat::RGBAFloat};
        // TODO: wrap-s/t/r == GL_CLAMP_TO_EDGE
        // TODO: ensure GL_TEXTURE_MIN_FILTER is GL_LINEAR_MIPMAP_LINEAR
        // TODO: ensure GL_TEXTURE_MAG_FILTER is GL_LINEAR
        // TODO: GL_TEXTURE_CUBE_MAP_SEAMLESS

        size_t const maxMipmapLevel = static_cast<size_t>(std::max(
            0,
            osc::bit_width(static_cast<size_t>(levelZeroWidth)) - 1
        ));
        static_assert(maxMipmapLevel == 7);

        // render prefilter map such that each supported level of roughness maps into one
        // LOD of the cubemap's mipmaps
        for (size_t mip = 0; mip <= maxMipmapLevel; ++mip)
        {
            size_t const mipWidth = levelZeroWidth >> mip;
            captureRT.setDimensions({static_cast<int>(mipWidth), static_cast<int>(mipWidth)});

            float const roughness = static_cast<float>(mip)/static_cast<float>(maxMipmapLevel);
            material.setFloat("uRoughness", roughness);

            osc::Graphics::DrawMesh(osc::GenCube(), osc::Transform{}, material, camera);
            camera.renderTo(captureRT);
            osc::Graphics::CopyTexture(captureRT, rv, mip);
        }

        return rv;
    }

    osc::Texture2D Create2DBRDFLookup()
    {
        osc::RenderTexture renderTex
        {
            {512, 512}
        };
        renderTex.setColorFormat(osc::RenderTextureFormat::ARGBFloat16);  // TODO RG16F in LearnOpenGL

        osc::Material material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/BRDF.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/BRDF.frag"),
            },
        };

        osc::Mesh quad = osc::GenTexturedQuad();

        // TODO: Graphics::Blit with material
        osc::Camera camera;
        camera.setProjectionMatrixOverride(glm::mat4{1.0f});
        camera.setViewMatrixOverride(glm::mat4{1.0f});

        osc::Graphics::DrawMesh(quad, osc::Transform{}, material, camera);
        camera.renderTo(renderTex);

        osc::Texture2D rv
        {
            {512, 512},
            osc::TextureFormat::RGBFloat,  // TODO: RG16F in LearnOpenGL
            osc::ColorSpace::Linear,
            osc::TextureWrapMode::Clamp,
            osc::TextureFilterMode::Linear,
        };
        osc::Graphics::CopyTexture(renderTex, rv);
        return rv;
    }

    osc::Material CreateMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/PBR.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/PBR.frag"),
            },
        };
        rv.setFloat("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRSpecularIrradianceTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
        m_IsMouseCaptured = false;
    }

    bool implOnEvent(SDL_Event const& e) final
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

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
        Rect const outputRect = GetMainViewportWorkspaceScreenRect();
        m_OutputRender.setDimensions(Dimensions(outputRect));
        m_OutputRender.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());

        updateCameraFromInputs();
        draw3DRender();
        drawBackground();
        Graphics::BlitToScreen(m_OutputRender, outputRect);
        draw2DUI();
        m_PerfPanel.onDraw();
    }

    void updateCameraFromInputs()
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

        for (int row = 0; row < c_NumRows; ++row)
        {
            m_PBRMaterial.setFloat("uMetallicity", static_cast<float>(row) / static_cast<float>(c_NumRows));

            for (int col = 0; col < c_NumCols; ++col)
            {
                float const normalizedCol = static_cast<float>(col) / static_cast<float>(c_NumCols);
                m_PBRMaterial.setFloat("uRoughness", glm::clamp(normalizedCol, 0.005f, 1.0f));

                Transform t;
                t.position =
                {
                    (static_cast<float>(col) - static_cast<float>(c_NumCols)/2.0f) * c_CellSpacing,
                    (static_cast<float>(row) - static_cast<float>(c_NumRows)/2.0f) * c_CellSpacing,
                    0.0f
                };

                Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
            }
        }
    }

    void drawLights()
    {
        m_PBRMaterial.setVec3("uAlbedoColor", {1.0f, 1.0f, 1.0f});

        for (glm::vec3 const& pos : c_LightPositions)
        {
            Transform t;
            t.position = pos;
            t.scale = glm::vec3{0.5f};

            Graphics::DrawMesh(m_SphereMesh, t, m_PBRMaterial, m_Camera);
        }
    }

    void drawBackground()
    {
        m_BackgroundMaterial.setRenderTexture("uEnvironmentMap", m_ProjectedMap);
        m_BackgroundMaterial.setDepthFunction(DepthFunction::LessOrEqual);  // for skybox depth trick
        Graphics::DrawMesh(m_CubeMesh, Transform{}, m_BackgroundMaterial, m_Camera);
        m_Camera.setClearFlags(osc::CameraClearFlags::Nothing);
        m_Camera.renderTo(m_OutputRender);
        m_Camera.setClearFlags(osc::CameraClearFlags::Default);
    }

    void draw2DUI()
    {
        if (ImGui::Begin("Controls"))
        {
            float ao = m_PBRMaterial.getFloat("uAO").value_or(1.0f);
            if (ImGui::SliderFloat("ao", &ao, 0.0f, 1.0f))
            {
                m_PBRMaterial.setFloat("uAO", ao);
            }
        }
        ImGui::End();
    }

    Texture2D m_Texture = osc::LoadTexture2DFromImage(
        App::resource("textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap();
    RenderTexture m_IrradianceMap = CreateIrradianceCubemap(m_ProjectedMap);
    Cubemap m_PrefilterMap = CreatePreFilteredEnvironmentMap(m_ProjectedMap);
    Texture2D m_BRDFLookup = Create2DBRDFLookup();
    RenderTexture m_OutputRender{{1, 1}};

    Material m_BackgroundMaterial
    {
        Shader
        {
            App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/Skybox.vert"),
            App::slurp("shaders/LearnOpenGL/PBR/ibl_specular/Skybox.frag"),
        },
    };

    Mesh m_CubeMesh = GenCube();
    Material m_PBRMaterial = CreateMaterial();
    Mesh m_SphereMesh = GenSphere(64, 64);

    Camera m_Camera = CreateCamera();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = true;

    PerfPanel m_PerfPanel{"Perf"};
};


// public API

osc::CStringView osc::LOGLPBRSpecularIrradianceTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPBRSpecularIrradianceTab::LOGLPBRSpecularIrradianceTab(ParentPtr<TabHost> const&) :
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

osc::CStringView osc::LOGLPBRSpecularIrradianceTab::implGetName() const
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

void osc::LOGLPBRSpecularIrradianceTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPBRSpecularIrradianceTab::implOnDraw()
{
    m_Impl->onDraw();
}
