#include "cached_model_renderer.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair_info.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_graphics_helpers.h>
#include <libopynsim/graphics/overlay_decoration_generator.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/bvh.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/perf.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    bool IsContributorToSceneVolume(const osc::SceneDecoration& dec)
    {
        if (dec.flags & osc::SceneDecorationFlag::NoSceneVolumeContribution) {
            // If this flag is set, then the decoration shouldn't contribute to
            // the scene's volume - even if it's visible (#1071).
            return false;
        }

        // If it's a decoration that's either fully drawn or a wireframe, it's part of the
        // scene's visible bounds (invisible objects may cast shadows, but they shouldn't be
        // considered part of the visible bounds, #1029).
        return
            (not (dec.flags & osc::SceneDecorationFlag::NoDrawInScene)) or
            (dec.flags & osc::SceneDecorationFlag::DrawWireframeOverlay);
    }

    // cache for decorations generated from a model+state+params
    class CachedDecorationState final {
    public:
        explicit CachedDecorationState(std::shared_ptr<osc::SceneCache> meshCache_) :
            m_MeshCache{std::move(meshCache_)}
        {
        }

        bool update(
            const opyn::ModelStatePair& modelState,
            const ModelRendererParams& params)
        {
            OSC_PERF("CachedModelRenderer/generateDecorationsCached");

            const ModelStatePairInfo info{modelState};
            if (info != m_PrevModelStateInfo ||
                params.decoration_options != m_PrevDecorationOptions ||
                params.overlay_options != m_PrevOverlayOptions)
            {
                m_Drawlist.clear();
                m_BVH.clear();
                m_SceneVolume.reset();

                // regenerate
                const auto onComponentDecoration = [this](const OpenSim::Component&, osc::SceneDecoration&& dec)
                {
                    if (IsContributorToSceneVolume(dec)) {
                        m_SceneVolume = osc::bounding_aabb_of(m_SceneVolume, dec.world_space_bounds());
                    }
                    m_Drawlist.push_back(std::move(dec));
                };
                generate_decorations(
                    *m_MeshCache,
                    modelState,
                    params.decoration_options,
                    onComponentDecoration
                );
                osc::update_scene_bvh(m_Drawlist, m_BVH);

                const auto onOverlayDecoration = [this](osc::SceneDecoration&& dec)
                {
                    m_Drawlist.push_back(std::move(dec));
                };
                generate_overlay_decorations(
                    *m_MeshCache,
                    params.overlay_options,
                    m_BVH,
                    modelState.get_fixup_scale_factor(),
                    onOverlayDecoration
                );

                m_PrevModelStateInfo = info;
                m_PrevDecorationOptions = params.decoration_options;
                m_PrevOverlayOptions = params.overlay_options;
                return true;   // updated
            }
            else
            {
                return false;  // already up to date
            }
        }

        std::span<const osc::SceneDecoration> getDrawlist() const { return m_Drawlist; }
        const osc::BVH& getBVH() const { return m_BVH; }
        std::optional<osc::AABB> getAABB() const
        {
            return m_BVH.bounds();
        }
        std::optional<osc::AABB> getVisibleAABB() const
        {
            return m_SceneVolume;
        }
        osc::SceneCache& updSceneCache() const
        {
            // TODO: technically (imo) this breaks `const`
            return *m_MeshCache;
        }

    private:
        std::shared_ptr<osc::SceneCache> m_MeshCache;
        ModelStatePairInfo m_PrevModelStateInfo;
        OpenSimDecorationOptions m_PrevDecorationOptions;
        OverlayDecorationOptions m_PrevOverlayOptions;
        std::vector<osc::SceneDecoration> m_Drawlist;
        osc::BVH m_BVH;
        std::optional<osc::AABB> m_SceneVolume;
    };
}

class opyn::CachedModelRenderer::Impl final {
public:
    explicit Impl(const std::shared_ptr<osc::SceneCache>& cache) :
        m_DecorationCache{cache},
        m_Renderer{*cache}
    {}

    osc::RenderTexture& onDraw(
        const ModelStatePair& modelState,
        const ModelRendererParams& renderParams,
        osc::Vector2 dims,
        float devicePixelRatio,
        osc::AntiAliasingLevel antiAliasingLevel)
    {
        OSC_PERF("CachedModelRenderer/on_draw");

        // setup render/rasterization parameters
        const osc::SceneRendererParams rendererParameters = calc_scene_renderer_params(
            renderParams,
            dims,
            devicePixelRatio,
            antiAliasingLevel,
            modelState.get_fixup_scale_factor()
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

    osc::RenderTexture& updRenderTexture()
    {
        return m_Renderer.upd_render_texture();
    }

    std::span<const osc::SceneDecoration> getDrawlist() const
    {
        return m_DecorationCache.getDrawlist();
    }

    std::optional<osc::AABB> bounds() const
    {
        return m_DecorationCache.getAABB();
    }

    std::optional<osc::AABB> visibleBounds() const
    {
        return m_DecorationCache.getVisibleAABB();
    }

    std::optional<osc::AABB> visibleBounds(
        const ModelStatePair& modelState,
        const ModelRendererParams& params)
    {
        m_DecorationCache.update(modelState, params);
        return m_DecorationCache.getVisibleAABB();
    }

    std::optional<osc::SceneCollision> getClosestCollision(
        const ModelRendererParams& params,
        osc::Vector2 mouseScreenPosition,
        const osc::Rect& viewportScreenRect) const
    {
        return opyn::get_closest_collision(
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
    osc::SceneRendererParams m_PrevRendererParams;
    osc::SceneRenderer m_Renderer;
};


opyn::CachedModelRenderer::CachedModelRenderer(const std::shared_ptr<osc::SceneCache>& cache) :
    impl_{std::make_unique<Impl>(cache)}
{}
opyn::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
opyn::CachedModelRenderer& opyn::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
opyn::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

osc::RenderTexture& opyn::CachedModelRenderer::on_draw(
    const ModelStatePair& modelState,
    const ModelRendererParams& renderParams,
    osc::Vector2 dims,
    float devicePixelRatio,
    osc::AntiAliasingLevel antiAliasingLevel)
{
    return impl_->onDraw(
        modelState,
        renderParams,
        dims,
        devicePixelRatio,
        antiAliasingLevel
    );
}

osc::RenderTexture& opyn::CachedModelRenderer::upd_render_texture()
{
    return impl_->updRenderTexture();
}

std::span<const osc::SceneDecoration> opyn::CachedModelRenderer::get_drawlist() const
{
    return impl_->getDrawlist();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::bounds() const
{
    return impl_->bounds();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::visible_bounds() const
{
    return impl_->visibleBounds();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::visible_bounds(
    const ModelStatePair& modelState,
    const ModelRendererParams& renderParams)
{
    return impl_->visibleBounds(modelState, renderParams);
}

std::optional<osc::SceneCollision> opyn::CachedModelRenderer::get_closest_collision(
    const ModelRendererParams& params,
    osc::Vector2 mouseScreenPosition,
    const osc::Rect& viewportScreenRect) const
{
    return impl_->getClosestCollision(params, mouseScreenPosition, viewportScreenRect);
}
