#include "cached_model_renderer.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair_info.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_graphics_helpers.h>
#include <libopynsim/graphics/overlay_decoration_generator.h>
#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/bvh.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utilities/perf.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace osc;
using namespace opyn;

namespace
{
    bool IsContributorToSceneVolume(const SceneDecoration& dec)
    {
        if (dec.flags & SceneDecorationFlag::NoSceneVolumeContribution) {
            // If this flag is set, then the decoration shouldn't contribute to
            // the scene's volume - even if it's visible (#1071).
            return false;
        }

        // If it's a decoration that's either fully drawn or a wireframe, it's part of the
        // scene's visible bounds (invisible objects may cast shadows, but they shouldn't be
        // considered part of the visible bounds, #1029).
        return
            (not (dec.flags & SceneDecorationFlag::NoDrawInScene)) or
            (dec.flags & SceneDecorationFlag::DrawWireframeOverlay);
    }

    // cache for decorations generated from a model+state+params
    class CachedDecorationState final {
    public:
        explicit CachedDecorationState(std::shared_ptr<SceneCache> meshCache_) :
            m_MeshCache{std::move(meshCache_)}
        {
        }

        bool update(
            const ModelStatePair& modelState,
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
                m_SceneVolume.reset();

                // regenerate
                const auto onComponentDecoration = [this](const OpenSim::Component&, SceneDecoration&& dec)
                {
                    if (IsContributorToSceneVolume(dec)) {
                        m_SceneVolume = bounding_aabb_of(m_SceneVolume, dec.world_space_bounds());
                    }
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
        std::optional<AABB> getAABB() const
        {
            return m_BVH.bounds();
        }
        std::optional<AABB> getVisibleAABB() const
        {
            return m_SceneVolume;
        }
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
        std::optional<AABB> m_SceneVolume;
    };
}

class osc::CachedModelRenderer::Impl final {
public:
    explicit Impl(const std::shared_ptr<SceneCache>& cache) :
        m_DecorationCache{cache},
        m_Renderer{*cache}
    {}

    void autoFocusCamera(
        const ModelStatePair& modelState,
        ModelRendererParams& params,
        float aspectRatio)
    {
        m_DecorationCache.update(modelState, params);
        if (const std::optional<AABB> aabb = m_DecorationCache.getVisibleAABB()) {
            auto_focus(params.camera, *aabb, aspectRatio);
        }
    }

    RenderTexture& onDraw(
        const ModelStatePair& modelState,
        const ModelRendererParams& renderParams,
        Vector2 dims,
        float devicePixelRatio,
        AntiAliasingLevel antiAliasingLevel)
    {
        OSC_PERF("CachedModelRenderer/on_draw");

        // setup render/rasterization parameters
        const SceneRendererParams rendererParameters = CalcSceneRendererParams(
            renderParams,
            dims,
            devicePixelRatio,
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

    std::optional<AABB> visibleBounds() const
    {
        return m_DecorationCache.getVisibleAABB();
    }

    std::optional<SceneCollision> getClosestCollision(
        const ModelRendererParams& params,
        Vector2 mouseScreenPosition,
        const Rect& viewportScreenRect) const
    {
        return GetClosestCollision(
            m_DecorationCache.getBVH(),
            m_DecorationCache.updSceneCache(),
            m_DecorationCache.getDrawlist(),
            params.camera,
            mouseScreenPosition,
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
    const ModelStatePair& modelState,
    const ModelRendererParams& renderParams,
    Vector2 dims,
    float devicePixelRatio,
    AntiAliasingLevel antiAliasingLevel)
{
    return m_Impl->onDraw(
        modelState,
        renderParams,
        dims,
        devicePixelRatio,
        antiAliasingLevel
    );
}

void osc::CachedModelRenderer::autoFocusCamera(
    const ModelStatePair& modelState,
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

std::optional<AABB> osc::CachedModelRenderer::visibleBounds() const
{
    return m_Impl->visibleBounds();
}

std::optional<SceneCollision> osc::CachedModelRenderer::getClosestCollision(
    const ModelRendererParams& params,
    Vector2 mouseScreenPosition,
    const Rect& viewportScreenRect) const
{
    return m_Impl->getClosestCollision(params, mouseScreenPosition, viewportScreenRect);
}
