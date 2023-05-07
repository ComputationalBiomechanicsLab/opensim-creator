#include "RendererSSAOTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/ColorSpace.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Graphics/TextureWrapMode.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>

#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace
{
    osc::Camera CreateCameraWithSameParamsAsLearnOpenGL()
    {
        osc::Camera rv;
        rv.setPosition({0.0f, 0.0f, 5.0f});
        rv.setCameraFOV(glm::radians(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor(osc::Color::black());
        return rv;
    }

    std::vector<glm::vec3> GenerateSampleKernel(size_t numSamples)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> zeroToOne{0.0f, 1.0f};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<glm::vec3> rv;
        rv.reserve(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            // scale samples such that they are more aligned to
            // the center of the kernel
            float scale = static_cast<float>(i)/numSamples;
            scale = glm::mix(0.1f, 1.0f, scale*scale);

            glm::vec3 sample = {minusOneToOne(rng), minusOneToOne(rng), minusOneToOne(rng)};
            sample = glm::normalize(sample);
            sample *= zeroToOne(rng);
            sample *= scale;

            rv.push_back(sample);
        }

        return rv;
    }

    std::vector<glm::vec4> GenerateNoiseTexturePixels(size_t numPixels)
    {
        std::default_random_engine rng{std::random_device{}()};
        std::uniform_real_distribution<float> minusOneToOne{-1.0f, 1.0f};

        std::vector<glm::vec4> rv;
        rv.reserve(numPixels);
        for (size_t i = 0; i < numPixels; ++i)
        {
            rv.push_back(glm::vec4
            {
                minusOneToOne(rng),
                minusOneToOne(rng),
                0.0f,  // rotate around z-axis in tangent space
                0.0f,  // ignored (Texture2D doesn't support RGB --> RGBA upload conversion)
            });
        }
        return rv;
    }

    template<typename T>
    nonstd::span<uint8_t const> ToByteSpan(nonstd::span<T const> vs)
    {
        return
        {
            reinterpret_cast<uint8_t const*>(vs.data()),
            reinterpret_cast<uint8_t const*>(vs.data() + vs.size())
        };
    }

    osc::Texture2D GenerateNoiseTexture(glm::ivec2 dimensions)
    {
        std::vector<glm::vec4> const pixels =
            GenerateNoiseTexturePixels(static_cast<size_t>(dimensions.x * dimensions.y));

        osc::Texture2D rv
        {
            dimensions,
            osc::TextureFormat::RGBAFloat,
            ToByteSpan<glm::vec4>(pixels),
            osc::ColorSpace::Linear,
        };

        rv.setFilterMode(osc::TextureFilterMode::Nearest);
        rv.setWrapMode(osc::TextureWrapMode::Repeat);

        return rv;
    }
}

class osc::RendererSSAOTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " RendererSSAOTab";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
        m_IsMouseCaptured = false;
    }

    bool onEvent(SDL_Event const& e)
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

    void onTick()
    {
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    std::vector<glm::vec3> m_SampleKernel = GenerateSampleKernel(64);
    osc::Texture2D m_NoiseTexture = GenerateNoiseTexture({4, 4});
    glm::vec3 m_LightPosition = {2.0f, 4.0f, -2.0f};
    glm::vec3 m_LightColor = {0.2f, 0.2f, 0.7f};

    Camera m_Camera = CreateCameraWithSameParamsAsLearnOpenGL();
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};

    Mesh m_SphereMesh = GenUntexturedUVSphere(32, 32);
    Mesh m_CubeMesh = GenCube();
    Mesh m_QuadMesh = GenTexturedQuad();

    // TODO:
    //
    // - gbuffer (geometry pass)
    // - ssao pass
    // - blur pass
    // - lighting pass
};


// public API (PIMPL)

osc::CStringView osc::RendererSSAOTab::id() noexcept
{
    return "Renderer/SSAO";
}

osc::RendererSSAOTab::RendererSSAOTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererSSAOTab::RendererSSAOTab(RendererSSAOTab&&) noexcept = default;
osc::RendererSSAOTab& osc::RendererSSAOTab::operator=(RendererSSAOTab&&) noexcept = default;
osc::RendererSSAOTab::~RendererSSAOTab() noexcept = default;

osc::UID osc::RendererSSAOTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererSSAOTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererSSAOTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererSSAOTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererSSAOTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererSSAOTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererSSAOTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererSSAOTab::implOnDraw()
{
    m_Impl->onDraw();
}
