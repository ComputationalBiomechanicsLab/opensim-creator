#include "CachedModelRenderer.hpp"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.hpp>
#include <OpenSimCreator/Documents/Model/ModelStatePairInfo.hpp>
#include <OpenSimCreator/Graphics/ModelRendererParams.hpp>
#include <OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp>

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/Scene/SceneRenderer.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/Utils/Perf.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    // cache for decorations generated from a model+state+params
    class CachedDecorationState final {
    public:
        explicit CachedDecorationState(std::shared_ptr<SceneCache> meshCache_) :
            m_MeshCache{std::move(meshCache_)}
        {
        }

        bool update(
            IConstModelStatePair const& modelState,
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
                auto const onComponentDecoration = [this](OpenSim::Component const&, SceneDecoration&& dec)
                {
                    m_Drawlist.push_back(std::move(dec));
                };
                GenerateDecorations(
                    *m_MeshCache,
                    modelState,
                    params.decorationOptions,
                    onComponentDecoration
                );
                UpdateSceneBVH(m_Drawlist, m_BVH);

                auto const onOverlayDecoration = [this](SceneDecoration&& dec)
                {
                    m_Drawlist.push_back(std::move(dec));
                };
                GenerateOverlayDecorations(
                    *m_MeshCache,
                    params.overlayOptions,
                    m_BVH,
                    onOverlayDecoration
                );

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

        std::span<SceneDecoration const> getDrawlist() const { return m_Drawlist; }
        BVH const& getBVH() const { return m_BVH; }
        std::optional<AABB> getAABB() const { return m_BVH.getRootAABB(); }
        SceneCache& updSceneCache() const
        {
            // TODO: technically (imo) this breaks `const`
            return *m_MeshCache;
        }

    private:
        std::shared_ptr<SceneCache> m_MeshCache;
        ModelStatePairInfo m_PrevModelStateInfo;
        OpenSimDecorationOptions m_PrevDecorationOptions;
        OverlayDecorationOptions m_PrevOverlayOptions;
        std::vector<SceneDecoration> m_Drawlist;
        BVH m_BVH;
    };
}

class osc::CachedModelRenderer::Impl final {
public:
    Impl(
        AppConfig const& config,
        std::shared_ptr<SceneCache> const& meshCache,
        ShaderCache& shaderCache) :

        m_DecorationCache{meshCache},
        m_Renderer{config, *meshCache, shaderCache}
    {
    }

    void autoFocusCamera(
        IConstModelStatePair const& modelState,
        ModelRendererParams& params,
        float aspectRatio)
    {
        m_DecorationCache.update(modelState, params);
        if (std::optional<AABB> const aabb = m_DecorationCache.getAABB())
        {
            AutoFocus(params.camera, *aabb, aspectRatio);
        }
    }

    RenderTexture& onDraw(
        IConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams,
        Vec2 dims,
        AntiAliasingLevel antiAliasingLevel)
    {
        OSC_PERF("CachedModelRenderer/onDraw");

        // setup render/rasterization parameters
        SceneRendererParams const rendererParameters = CalcSceneRendererParams(
            renderParams,
            dims,
            antiAliasingLevel,
            modelState.getFixupScaleFactor()
        );

        // if the decorations or rendering params have changed, re-render
        if (m_DecorationCache.update(modelState, renderParams) ||
            rendererParameters != m_PrevRendererParams)
        {
            OSC_PERF("CachedModelRenderer/onDraw/render");
            m_Renderer.render(m_DecorationCache.getDrawlist(), rendererParameters);
            m_PrevRendererParams = rendererParameters;
        }

        return m_Renderer.updRenderTexture();
    }

    RenderTexture& updRenderTexture()
    {
        return m_Renderer.updRenderTexture();
    }

    std::span<SceneDecoration const> getDrawlist() const
    {
        return m_DecorationCache.getDrawlist();
    }

    std::optional<AABB> getRootAABB() const
    {
        return m_DecorationCache.getAABB();
    }

    std::optional<SceneCollision> getClosestCollision(
        ModelRendererParams const& params,
        Vec2 mouseScreenPos,
        Rect const& viewportScreenRect) const
    {
        return GetClosestCollision(
            m_DecorationCache.getBVH(),
            m_DecorationCache.updSceneCache(),
            m_DecorationCache.getDrawlist(),
            params.camera,
            mouseScreenPos,
            viewportScreenRect
        );
    }

private:
    CachedDecorationState m_DecorationCache;
    SceneRendererParams m_PrevRendererParams;
    SceneRenderer m_Renderer;
};


// public API (PIMPL)

osc::CachedModelRenderer::CachedModelRenderer(
    AppConfig const& config,
    std::shared_ptr<SceneCache> const& meshCache,
    ShaderCache& shaderCache) :

    m_Impl{std::make_unique<Impl>(config, meshCache, shaderCache)}
{
}
osc::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer& osc::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

RenderTexture& osc::CachedModelRenderer::onDraw(
    IConstModelStatePair const& modelState,
    ModelRendererParams const& renderParams,
    Vec2 dims,
    AntiAliasingLevel antiAliasingLevel)
{
    return m_Impl->onDraw(
        modelState,
        renderParams,
        dims,
        antiAliasingLevel
    );
}

void osc::CachedModelRenderer::autoFocusCamera(
    IConstModelStatePair const& modelState,
    ModelRendererParams& renderParams,
    float aspectRatio)
{
    m_Impl->autoFocusCamera(modelState, renderParams, aspectRatio);
}

RenderTexture& osc::CachedModelRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}

std::span<SceneDecoration const> osc::CachedModelRenderer::getDrawlist() const
{
    return m_Impl->getDrawlist();
}

std::optional<AABB> osc::CachedModelRenderer::getRootAABB() const
{
    return m_Impl->getRootAABB();
}

std::optional<SceneCollision> osc::CachedModelRenderer::getClosestCollision(
    ModelRendererParams const& params,
    Vec2 mouseScreenPos,
    Rect const& viewportScreenRect) const
{
    return m_Impl->getClosestCollision(params, mouseScreenPos, viewportScreenRect);
}
