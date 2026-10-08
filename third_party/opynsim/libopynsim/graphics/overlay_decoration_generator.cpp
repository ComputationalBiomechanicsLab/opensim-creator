#include "overlay_decoration_generator.h"

#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/scene/scene_helpers.h>

#include <functional>

void opyn::generate_overlay_decorations(
    osc::SceneCache& meshCache,
    const OverlayDecorationOptions& params,
    const osc::BVH& sceneBVH,
    float fixupScaleFactor,
    const std::function<void(osc::SceneDecoration&&)>& out)
{
    if (params.get_draw_aabbs()) {
        draw_bvh_leaf_nodes(meshCache, sceneBVH, out);
    }

    if (params.get_draw_bvh()) {
        draw_bvh(meshCache, sceneBVH, out);
    }

    if (params.get_draw_xz_grid()) {
        draw_xz_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_xy_grid()) {
        draw_xy_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_yz_grid()) {
        draw_yz_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.get_draw_axis_lines()) {
        draw_xz_floor_lines(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }
}
