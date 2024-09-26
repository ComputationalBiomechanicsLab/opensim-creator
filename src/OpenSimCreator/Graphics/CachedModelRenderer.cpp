#include "CachedModelRenderer.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/Model/ModelStatePairInfo.h>
#include <OpenSimCreator/Graphics/ModelRendererParams.h>
#include <OpenSimCreator/Graphics/OpenSimGraphicsHelpers.h>
#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.h>

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/Perf.h>

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
            const IModelStatePair& modelState,
            const ModelRendererParams& params)
        {
            OSC_PERF("CachedModelRenderer/generateDecorationsCached");

            const ModelStatePairInfo info{modelState};
            if (info != m_PrevModelStateInfo ||
                params.decorationOptions != m_PrevDecorationOptions ||
                params.overlayOptions != m_PrevOverlayOptions)
            {
                m_Drawlist.clear();
                m_BVH.clear();

                // regenerate
                const auto onComponentDecoration = [this](const OpenSim::Component&, SceneDecoration&& dec)
                {
                    m_Drawlist.push_back(std::move(dec));
                };
                GenerateDecorations(
                    *m_MeshCache,
                    modelState,
                    params.decorationOptions,
                    onComponentDecoration
                );
                update_scene_bvh(m_Drawlist, m_BVH);

                const auto onOverlayDecoration = [this](SceneDecoration&& dec)
                {
                    m_Drawlist.push_back(std::move(dec));
                };
                GenerateOverlayDecorations(
                    *m_MeshCache,
                    params.overlayOptions,
                    m_BVH,
                    modelState.getFixupScaleFactor(),
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

        std::span<const SceneDecoration> getDrawlist() const { return m_Drawlist; }
        const BVH& getBVH() const { return m_BVH; }
        std::optional<AABB> getAABB() const { return m_BVH.bounds(); }
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
    explicit Impl(const std::shared_ptr<SceneCache>& cache) :
        m_DecorationCache{cache},
        m_Renderer{*cache}
    {}

    void autoFocusCamera(
        const IModelStatePair& modelState,
        ModelRendererParams& params,
        float aspectRatio)
    {
        m_DecorationCache.update(modelState, params);
        if (const std::optional<AABB> aabb = m_DecorationCache.getAABB()) {
            auto_focus(params.camera, *aabb, aspectRatio);
        }
    }

    RenderTexture& onDraw(
        const IModelStatePair& modelState,
        const ModelRendererParams& renderParams,
        Vec2 dims,
        AntiAliasingLevel antiAliasingLevel)
    {
        OSC_PERF("CachedModelRenderer/on_draw");

        // setup render/rasterization parameters
        const SceneRendererParams rendererParameters = CalcSceneRendererParams(
            renderParams,
            dims,
            antiAliasingLevel,
            modelState.getFixupScaleFactor()
        );

        // if the decorations or rendering params have changed, re-render
        if (m_DecorationCache.update(modelState, renderParams) ||
            rendererParameters != m_PrevRendererParams)
        {
            OSC_PERF("CachedModelRenderer/on_draw/render");
            m_Renderer.render(m_DecorationCache.getDrawlist(), rendererParameters);
            m_PrevRendererParams = rendererParameters;
        }

        return m_Renderer.upd_render_texture();
    }

    RenderTexture& updRenderTexture()
    {
        return m_Renderer.upd_render_texture();
    }

    std::span<const SceneDecoration> getDrawlist() const
    {
        return m_DecorationCache.getDrawlist();
    }

    std::optional<AABB> bounds() const
    {
        return m_DecorationCache.getAABB();
    }

    std::optional<SceneCollision> getClosestCollision(
        const ModelRendererParams& params,
        Vec2 mouseScreenPos,
        const Rect& viewportScreenRect) const
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


osc::CachedModelRenderer::CachedModelRenderer(const std::shared_ptr<SceneCache>& cache) :
    m_Impl{std::make_unique<Impl>(cache)}
{}
osc::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer& osc::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

RenderTexture& osc::CachedModelRenderer::onDraw(
    const IModelStatePair& modelState,
    const ModelRendererParams& renderParams,
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
    const IModelStatePair& modelState,
    ModelRendererParams& renderParams,
    float aspectRatio)
{
    m_Impl->autoFocusCamera(modelState, renderParams, aspectRatio);
}

RenderTexture& osc::CachedModelRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}

std::span<const SceneDecoration> osc::CachedModelRenderer::getDrawlist() const
{
    return m_Impl->getDrawlist();
}

std::optional<AABB> osc::CachedModelRenderer::bounds() const
{
    return m_Impl->bounds();
}

std::optional<SceneCollision> osc::CachedModelRenderer::getClosestCollision(
    const ModelRendererParams& params,
    Vec2 mouseScreenPos,
    const Rect& viewportScreenRect) const
{
    return m_Impl->getClosestCollision(params, mouseScreenPos, viewportScreenRect);
}
