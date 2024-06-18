#include "OverlayDecorationGenerator.h"

#include <OpenSimCreator/Graphics/OverlayDecorationOptions.h>

#include <oscar/Graphics/Scene/SceneHelpers.h>

#include <functional>

void osc::GenerateOverlayDecorations(
    SceneCache& meshCache,
    const OverlayDecorationOptions& params,
    const BVH& sceneBVH,
    const std::function<void(SceneDecoration&&)>& out)
{
    if (params.getDrawAABBs())
    {
        draw_bvh_leaf_nodes(meshCache, sceneBVH, out);
    }

    if (params.getDrawBVH())
    {
        draw_bvh(meshCache, sceneBVH, out);
    }

    if (params.getDrawXZGrid())
    {
        draw_xz_grid(meshCache, out);
    }

    if (params.getDrawXYGrid())
    {
        draw_xy_grid(meshCache, out);
    }

    if (params.getDrawYZGrid())
    {
        draw_yz_grid(meshCache, out);
    }

    if (params.getDrawAxisLines())
    {
        draw_xz_floor_lines(meshCache, out);
    }
}
