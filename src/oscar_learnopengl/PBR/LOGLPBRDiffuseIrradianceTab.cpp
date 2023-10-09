#include "LOGLPBRDiffuseIrradianceTab.hpp"

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
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Platform/App.hpp>
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
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/DiffuseIrradiance";

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
        osc::Texture2D hdrTexture = osc::LoadTexture2DFromImage(
            osc::App::resource("textures/hdr/newport_loft.hdr"),
            osc::ColorSpace::Linear,
            osc::ImageLoadingFlags::FlipVertically
        );
        hdrTexture.setWrapMode(osc::TextureWrapMode::Clamp);
        hdrTexture.setFilterMode(osc::TextureFilterMode::Linear);

        osc::RenderTexture cubemapRenderTarget{{512, 512}};
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
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/EquirectangularToCubemap.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/EquirectangularToCubemap.geom"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/EquirectangularToCubemap.frag"),
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
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/Convolution.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/Convolution.geom"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/Convolution.frag"),
            }
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

    osc::Material CreateMaterial()
    {
        osc::Material rv
        {
            osc::Shader
            {
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/PBR.vert"),
                osc::App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/PBR.frag"),
            },
        };
        rv.setFloat("uAO", 1.0f);
        return rv;
    }
}

class osc::LOGLPBRDiffuseIrradianceTab::Impl final : public osc::StandardTabBase {
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
        updateCameraFromInputs();
        draw3DRender();
        drawBackground();
        draw2DUI();
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
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        m_PBRMaterial.setVec3("uCameraWorldPos", m_Camera.getPosition());
        m_PBRMaterial.setVec3Array("uLightPositions", c_LightPositions);
        m_PBRMaterial.setVec3Array("uLightColors", c_LightRadiances);
        m_PBRMaterial.setRenderTexture("uIrradianceMap", m_IrradianceMap);

        drawSpheres();
        drawLights();

        m_Camera.renderToScreen();
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
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.setClearFlags(osc::CameraClearFlags::Nothing);
        m_Camera.renderToScreen();
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

    Material m_BackgroundMaterial
    {
        Shader
        {
            App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/Background.vert"),
            App::slurp("shaders/LearnOpenGL/PBR/diffuse_irradiance/Background.frag"),
        }
    };

    Mesh m_CubeMesh = GenCube();
    Material m_PBRMaterial = CreateMaterial();
    Mesh m_SphereMesh = GenSphere(64, 64);

    Camera m_Camera = CreateCamera();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = true;
};


// public API

osc::CStringView osc::LOGLPBRDiffuseIrradianceTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRDiffuseIrradianceTab::LOGLPBRDiffuseIrradianceTab(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab& osc::LOGLPBRDiffuseIrradianceTab::operator=(LOGLPBRDiffuseIrradianceTab&&) noexcept = default;
osc::LOGLPBRDiffuseIrradianceTab::~LOGLPBRDiffuseIrradianceTab() noexcept = default;

osc::UID osc::LOGLPBRDiffuseIrradianceTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLPBRDiffuseIrradianceTab::implGetName() const
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

void osc::LOGLPBRDiffuseIrradianceTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPBRDiffuseIrradianceTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPBRDiffuseIrradianceTab::implOnDraw()
{
    m_Impl->onDraw();
}
