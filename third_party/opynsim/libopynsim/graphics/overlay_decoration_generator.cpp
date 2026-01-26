#include "overlay_decoration_generator.h"

#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/scene/scene_helpers.h>

#include <functional>

void opyn::GenerateOverlayDecorations(
    osc::SceneCache& meshCache,
    const OverlayDecorationOptions& params,
    const osc::BVH& sceneBVH,
    float fixupScaleFactor,
    const std::function<void(osc::SceneDecoration&&)>& out)
{
    if (params.getDrawAABBs()) {
        draw_bvh_leaf_nodes(meshCache, sceneBVH, out);
    }

    if (params.getDrawBVH()) {
        draw_bvh(meshCache, sceneBVH, out);
    }

    if (params.getDrawXZGrid()) {
        draw_xz_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.getDrawXYGrid()) {
        draw_xy_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.getDrawYZGrid()) {
        draw_yz_grid(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }

    if (params.getDrawAxisLines()) {
        draw_xz_floor_lines(meshCache, [&out, fixupScaleFactor](osc::SceneDecoration&& dec)
        {
            dec.transform.scale *= fixupScaleFactor;
            out(std::move(dec));
        });
    }
}
