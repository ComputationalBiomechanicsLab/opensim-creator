#include "CachedModelRenderer.hpp"

#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp"
#include "OpenSimCreator/ModelStatePairInfo.hpp"
#include "OpenSimCreator/VirtualConstModelStatePair.hpp"

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneRenderer.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Utils/Perf.hpp>

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class osc::CachedModelRenderer::Impl final {
public:
    Impl(
        Config const& config,
        std::shared_ptr<MeshCache> meshCache,
        ShaderCache& shaderCache) :

        m_MeshCache{meshCache},
        m_Renderer{config, *meshCache, shaderCache}
    {
    }

    void autoFocusCamera(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams& params,
        float aspectRatio)
    {
        generateDecorationsCached(modelState, params);
        if (std::optional<AABB> aabb = m_BVH.getRootAABB())
        {
            AutoFocus(params.camera, *aabb, aspectRatio);
        }
    }

    RenderTexture& draw(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams,
        glm::vec2 dims,
        int32_t samples)
    {
        OSC_PERF("CachedModelRenderer/draw");

        // setup render/rasterization parameters
        osc::SceneRendererParams const rendererParameters = CalcSceneRendererParams(
            renderParams,
            dims,
            samples,            
            modelState.getFixupScaleFactor()
        );

        // if the decorations or rendering params have changed, re-render
        if (generateDecorationsCached(modelState, renderParams) ||
            rendererParameters != m_PrevRendererParams)
        {
            OSC_PERF("CachedModelRenderer/draw/render");
            m_Renderer.draw(m_Drawlist, rendererParameters);
            m_PrevRendererParams = rendererParameters;
        }

        return m_Renderer.updRenderTexture();
    }

    RenderTexture& updRenderTexture()
    {
        return m_Renderer.updRenderTexture();
    }

    nonstd::span<SceneDecoration const> getDrawlist() const
    {
        return m_Drawlist;
    }

    std::optional<AABB> getRootAABB() const
    {
        return m_BVH.getRootAABB();
    }

    std::optional<SceneCollision> getClosestCollision(
        ModelRendererParams const& params,
        glm::vec2 mouseScreenPos,
        Rect const& viewportScreenRect) const
    {
        return GetClosestCollision(
            m_BVH,
            m_Drawlist,
            params.camera,
            mouseScreenPos,
            viewportScreenRect
        );
    }

private:
    bool generateDecorationsCached(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& params)
    {
        OSC_PERF("CachedModelRenderer/generateDecorationsCached");

        ModelStatePairInfo const info{modelState};
        if (info != m_PrevModelStateInfo ||
            params.decorationOptions != m_PrevDecorationOptions ||
            params.overlayOptions != m_PrevOverlayOptions)
        {
            m_Drawlist.clear();
            m_BVH.clear();

            // regenerate
            auto onComponentDecoration = [this](OpenSim::Component const&, SceneDecoration&& dec)
            {
                m_Drawlist.push_back(std::move(dec));
            };
            auto onOverlayDecoration = [this](SceneDecoration&& dec)
            {
                m_Drawlist.push_back(std::move(dec));
            };

            GenerateDecorations(modelState, params.decorationOptions, *m_MeshCache, onComponentDecoration);
            UpdateSceneBVH(m_Drawlist, m_BVH);
            GenerateOverlayDecorations(*m_MeshCache, params.overlayOptions, m_BVH, onOverlayDecoration);

            m_PrevModelStateInfo = info;
            m_PrevDecorationOptions = params.decorationOptions;
            m_PrevOverlayOptions = params.overlayOptions;
            return true;   // updated
        }
        else
        {
            return false;  // already up to date
        }
    }

    std::shared_ptr<MeshCache> m_MeshCache;
    ModelStatePairInfo m_PrevModelStateInfo;
    OpenSimDecorationOptions m_PrevDecorationOptions;
    OverlayDecorationOptions m_PrevOverlayOptions;
    std::vector<SceneDecoration> m_Drawlist;
    BVH m_BVH;
    SceneRendererParams m_PrevRendererParams;
    SceneRenderer m_Renderer;
};


// public API (PIMPL)

osc::CachedModelRenderer::CachedModelRenderer(
    Config const& config,
    std::shared_ptr<MeshCache> meshCache,
    ShaderCache& shaderCache) :

    m_Impl{std::make_unique<Impl>(config, std::move(meshCache), shaderCache)}
{
}
osc::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer& osc::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

osc::RenderTexture& osc::CachedModelRenderer::draw(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams const& renderParams,
    glm::vec2 dims,
    int32_t samples)
{
    return m_Impl->draw(
        modelState,
        renderParams,
        dims,
        samples
    );
}

void osc::CachedModelRenderer::autoFocusCamera(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams& renderParams,
    float aspectRatio)
{
    m_Impl->autoFocusCamera(modelState, renderParams, aspectRatio);
}

osc::RenderTexture& osc::CachedModelRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}

nonstd::span<osc::SceneDecoration const> osc::CachedModelRenderer::getDrawlist() const
{
    return m_Impl->getDrawlist();
}

std::optional<osc::AABB> osc::CachedModelRenderer::getRootAABB() const
{
    return m_Impl->getRootAABB();
}

std::optional<osc::SceneCollision> osc::CachedModelRenderer::getClosestCollision(
    ModelRendererParams const& params,
    glm::vec2 mouseScreenPos,
    Rect const& viewportScreenRect) const
{
    return m_Impl->getClosestCollision(params, mouseScreenPos, viewportScreenRect);
}