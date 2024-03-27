#include "OverlayDecorationGenerator.h"

#include <OpenSimCreator/Graphics/OverlayDecorationOptions.h>

#include <oscar/Graphics/Scene/SceneHelpers.h>

#include <functional>

void osc::GenerateOverlayDecorations(
    SceneCache& meshCache,
    OverlayDecorationOptions const& params,
    BVH const& sceneBVH,
    std::function<void(SceneDecoration&&)> const& out)
{
    if (params.getDrawAABBs())
    {
        drawBVHLeafNodes(meshCache, sceneBVH, out);
    }

    if (params.getDrawBVH())
    {
        drawBVH(meshCache, sceneBVH, out);
    }

    if (params.getDrawXZGrid())
    {
        drawXZGrid(meshCache, out);
    }

    if (params.getDrawXYGrid())
    {
        drawXYGrid(meshCache, out);
    }

    if (params.getDrawYZGrid())
    {
        drawYZGrid(meshCache, out);
    }

    if (params.getDrawAxisLines())
    {
        drawXZFloorLines(meshCache, out);
    }
}
