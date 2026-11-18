#pragma once

#include <functional>

namespace osc
{
    class BVH;
    class SceneCache;
    struct SceneDecoration;
}

namespace opyn
{
    class OverlayDecorationOptions;

    // generates 3D overlays for the given options and passes them to the
    // output consumer
    void generate_overlay_decorations(
        osc::SceneCache&,
        const OverlayDecorationOptions&,
        const osc::BVH& scene_bvh,
        float fixup_scale_factor,
        const std::function<void(osc::SceneDecoration&&)>& out);
}
