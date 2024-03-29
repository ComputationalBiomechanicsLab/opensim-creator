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
        DrawBVHLeafNodes(meshCache, sceneBVH, out);
    }

    if (params.getDrawBVH())
    {
        DrawBVH(meshCache, sceneBVH, out);
    }

    if (params.getDrawXZGrid())
    {
        DrawXZGrid(meshCache, out);
    }

    if (params.getDrawXYGrid())
    {
        DrawXYGrid(meshCache, out);
    }

    if (params.getDrawYZGrid())
    {
        DrawYZGrid(meshCache, out);
    }

    if (params.getDrawAxisLines())
    {
        DrawXZFloorLines(meshCache, out);
    }
}
