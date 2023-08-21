#include "LOGLHDREquirectangularTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Cubemap.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/ImageLoadingFlags.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/RenderTextureFormat.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Graphics/TextureWrapMode.hpp"
#include "oscar/Graphics/TextureFilterMode.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/HDREquirectangular";

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
                osc::App::slurp("shaders/ExperimentEquirectangular.vert"),
                osc::App::slurp("shaders/ExperimentEquirectangular.geom"),
                osc::App::slurp("shaders/ExperimentEquirectangular.frag"),
            }
        };
        material.setTexture("uEquirectangularMap", hdrTexture);
        material.setMat4Array(
            "uShadowMatrices",
            osc::CalcCubemapViewProjMatrices(projectionMatrix, glm::vec3{})
        );

        osc::Camera camera;
        camera.setBackgroundColor(osc::Color::red());
        osc::Graphics::DrawMesh(osc::GenCube(), osc::Transform{}, material, camera);
        camera.renderTo(cubemapRenderTarget);

        return cubemapRenderTarget;
    }
}

class osc::LOGLHDREquirectangularTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
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
        drawBackground();
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

    void drawBackground()
    {
        m_BackgroundMaterial.setRenderTexture("uEnvironmentMap", m_ProjectedMap);
        m_BackgroundMaterial.setDepthFunction(DepthFunction::LessOrEqual);  // for skybox depth trick
        Graphics::DrawMesh(m_CubeMesh, Transform{}, m_BackgroundMaterial, m_Camera);
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    Texture2D m_Texture = osc::LoadTexture2DFromImage(
        App::resource("textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );

    RenderTexture m_ProjectedMap = LoadEquirectangularHDRTextureIntoCubemap();

    Material m_BackgroundMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentEquirectangularBackground.vert"),
            App::slurp("shaders/ExperimentEquirectangularBackground.frag"),
        }
    };

    Mesh m_CubeMesh = osc::GenCube();

    Camera m_Camera = CreateCamera();
    glm::vec3 m_CameraEulers = {};
    bool m_IsMouseCaptured = true;
};


// public API

osc::CStringView osc::LOGLHDREquirectangularTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLHDREquirectangularTab::LOGLHDREquirectangularTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLHDREquirectangularTab::LOGLHDREquirectangularTab(LOGLHDREquirectangularTab&&) noexcept = default;
osc::LOGLHDREquirectangularTab& osc::LOGLHDREquirectangularTab::operator=(LOGLHDREquirectangularTab&&) noexcept = default;
osc::LOGLHDREquirectangularTab::~LOGLHDREquirectangularTab() noexcept = default;

osc::UID osc::LOGLHDREquirectangularTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLHDREquirectangularTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHDREquirectangularTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLHDREquirectangularTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLHDREquirectangularTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLHDREquirectangularTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLHDREquirectangularTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLHDREquirectangularTab::implOnDraw()
{
    m_Impl->onDraw();
}
