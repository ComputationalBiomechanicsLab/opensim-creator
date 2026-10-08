#include "overlay_decoration_generator.h"

#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/scene/scene_helpers.h>

#include <functional>

void opyn::generate_overlay_decorations(
    osc::SceneCache& mesh_cache,
    const OverlayDecorationOptions& params,
    const osc::BVH& scene_bvh,
    float fixup_scale_factor,
    const std::function<void(osc::SceneDecoration&&)>& out)
{
    if (params.get_draw_aabbs()) {
        draw_bvh_leaf_nodes(mesh_cache, scene_bvh, out);
    }

    if (params.get_draw_bvh()) {
        draw_bvh(mesh_cache, scene_bvh, out);
    }

    if (params.get_draw_xz_grid()) {
        draw_xz_grid(mesh_cache, [&out, fixup_scale_factor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixup_scale_factor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_xy_grid()) {
        draw_xy_grid(mesh_cache, [&out, fixup_scale_factor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixup_scale_factor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_yz_grid()) {
        draw_yz_grid(mesh_cache, [&out, fixup_scale_factor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixup_scale_factor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_axis_lines()) {
        draw_xz_floor_lines(mesh_cache, [&out, fixup_scale_factor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixup_scale_factor;
            out(std::move(dec));
        });
    }
}
