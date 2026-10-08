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
    bool is_contributor_to_scene_volume(const osc::SceneDecoration& dec)
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
        explicit CachedDecorationState(std::shared_ptr<osc::SceneCache> mesh_cache) :
            mesh_cache_{std::move(mesh_cache)}
        {}

        bool update(
            const opyn::ModelStatePair& model_state,
            const ModelRendererParams& params)
        {
            OSC_PERF("CachedModelRenderer/generateDecorationsCached");

            const ModelStatePairInfo info{model_state};
            if (info != prev_model_state_info_ ||
                params.decoration_options != prev_decoration_options_ ||
                params.overlay_options != prev_overlay_options_)
            {
                drawlist_.clear();
                bvh_.clear();
                scene_bounds_.reset();

                // regenerate
                const auto on_component_decoration = [this](const OpenSim::Component&, osc::SceneDecoration&& dec)
                {
                    if (is_contributor_to_scene_volume(dec)) {
                        scene_bounds_ = osc::bounding_aabb_of(scene_bounds_, dec.world_space_bounds());
                    }
                    drawlist_.push_back(std::move(dec));
                };
                generate_decorations(
                    *mesh_cache_,
                    model_state,
                    params.decoration_options,
                    on_component_decoration
                );
                osc::update_scene_bvh(drawlist_, bvh_);

                const auto on_overlay_decoration = [this](osc::SceneDecoration&& dec)
                {
                    drawlist_.push_back(std::move(dec));
                };
                generate_overlay_decorations(
                    *mesh_cache_,
                    params.overlay_options,
                    bvh_,
                    model_state.get_fixup_scale_factor(),
                    on_overlay_decoration
                );

                prev_model_state_info_ = info;
                prev_decoration_options_ = params.decoration_options;
                prev_overlay_options_ = params.overlay_options;
                return true;   // updated
            }
            else
            {
                return false;  // already up to date
            }
        }

        std::span<const osc::SceneDecoration> get_drawlist() const { return drawlist_; }
        const osc::BVH& get_bvh() const { return bvh_; }
        std::optional<osc::AABB> get_aabb() const
        {
            return bvh_.bounds();
        }
        std::optional<osc::AABB> get_visible_aabb() const
        {
            return scene_bounds_;
        }
        osc::SceneCache& upd_scene_cache() const
        {
            // TODO: technically (imo) this breaks `const`
            return *mesh_cache_;
        }

    private:
        std::shared_ptr<osc::SceneCache> mesh_cache_;
        ModelStatePairInfo prev_model_state_info_;
        OpenSimDecorationOptions prev_decoration_options_;
        OverlayDecorationOptions prev_overlay_options_;
        std::vector<osc::SceneDecoration> drawlist_;
        osc::BVH bvh_;
        std::optional<osc::AABB> scene_bounds_;
    };
}

class opyn::CachedModelRenderer::Impl final {
public:
    explicit Impl(const std::shared_ptr<osc::SceneCache>& cache) :
        decoration_cache_{cache},
        renderer_{*cache}
    {}

    osc::RenderTexture& on_draw(
        const ModelStatePair& model_state,
        const ModelRendererParams& render_params,
        osc::Vector2 dims,
        float device_pixel_ratio,
        osc::AntiAliasingLevel anti_aliasing_level)
    {
        OSC_PERF("CachedModelRenderer/on_draw");

        // setup render/rasterization parameters
        const osc::SceneRendererParams renderer_parameters = calc_scene_renderer_params(
            render_params,
            dims,
            device_pixel_ratio,
            anti_aliasing_level,
            model_state.get_fixup_scale_factor()
        );

        // if the decorations or rendering params have changed, re-render
        if (decoration_cache_.update(model_state, render_params) ||
            renderer_parameters != prev_renderer_params_)
        {
            OSC_PERF("CachedModelRenderer/on_draw/render");
            renderer_.render(decoration_cache_.get_drawlist(), renderer_parameters);
            prev_renderer_params_ = renderer_parameters;
        }

        return renderer_.upd_render_texture();
    }

    osc::RenderTexture& upd_render_texture()
    {
        return renderer_.upd_render_texture();
    }

    std::span<const osc::SceneDecoration> get_drawlist() const
    {
        return decoration_cache_.get_drawlist();
    }

    std::optional<osc::AABB> bounds() const
    {
        return decoration_cache_.get_aabb();
    }

    std::optional<osc::AABB> visible_bounds() const
    {
        return decoration_cache_.get_visible_aabb();
    }

    std::optional<osc::AABB> visible_bounds(
        const ModelStatePair& model_state,
        const ModelRendererParams& params)
    {
        decoration_cache_.update(model_state, params);
        return decoration_cache_.get_visible_aabb();
    }

    std::optional<osc::SceneCollision> get_closest_collision(
        const ModelRendererParams& params,
        osc::Vector2 mouse_screen_position,
        const osc::Rect& viewport_screen_rect) const
    {
        return opyn::get_closest_collision(
            decoration_cache_.get_bvh(),
            decoration_cache_.upd_scene_cache(),
            decoration_cache_.get_drawlist(),
            params.camera,
            mouse_screen_position,
            viewport_screen_rect
        );
    }

private:
    CachedDecorationState decoration_cache_;
    osc::SceneRendererParams prev_renderer_params_;
    osc::SceneRenderer renderer_;
};


opyn::CachedModelRenderer::CachedModelRenderer(const std::shared_ptr<osc::SceneCache>& cache) :
    impl_{std::make_unique<Impl>(cache)}
{}
opyn::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
opyn::CachedModelRenderer& opyn::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
opyn::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

osc::RenderTexture& opyn::CachedModelRenderer::on_draw(
    const ModelStatePair& model_state,
    const ModelRendererParams& render_params,
    osc::Vector2 dims,
    float device_pixel_ratio,
    osc::AntiAliasingLevel anti_aliasing_level)
{
    return impl_->on_draw(
        model_state,
        render_params,
        dims,
        device_pixel_ratio,
        anti_aliasing_level
    );
}

osc::RenderTexture& opyn::CachedModelRenderer::upd_render_texture()
{
    return impl_->upd_render_texture();
}

std::span<const osc::SceneDecoration> opyn::CachedModelRenderer::get_drawlist() const
{
    return impl_->get_drawlist();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::bounds() const
{
    return impl_->bounds();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::visible_bounds() const
{
    return impl_->visible_bounds();
}

std::optional<osc::AABB> opyn::CachedModelRenderer::visible_bounds(
    const ModelStatePair& model_state,
    const ModelRendererParams& render_params)
{
    return impl_->visible_bounds(model_state, render_params);
}

std::optional<osc::SceneCollision> opyn::CachedModelRenderer::get_closest_collision(
    const ModelRendererParams& params,
    osc::Vector2 mouse_screen_position,
    const osc::Rect& viewport_screen_rect) const
{
    return impl_->get_closest_collision(params, mouse_screen_position, viewport_screen_rect);
}
