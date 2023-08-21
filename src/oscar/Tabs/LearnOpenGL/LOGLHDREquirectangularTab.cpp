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
            osc::CalcCubemapViewProjMatrices(projectionMatrix, {0.0f, 0.0f, 0.0f})
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
        OSC_ASSERT_ALWAYS(m_Texture.getTextureFormat() == TextureFormat::RGBFloat);
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
    }

    bool implOnEvent(SDL_Event const&) final
    {
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
        LoadEquirectangularHDRTextureIntoCubemap();
        Rect const r = GetMainViewportWorkspaceScreenRect();
        Graphics::BlitToScreen(m_Texture, r);
    }

    Texture2D m_Texture = osc::LoadTexture2DFromImage(
        App::resource("textures/hdr/newport_loft.hdr"),
        ColorSpace::Linear,
        ImageLoadingFlags::FlipVertically
    );
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
