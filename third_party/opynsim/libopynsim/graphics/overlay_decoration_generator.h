#pragma once

#include <functional>

namespace opyn { class OverlayDecorationOptions; }
namespace osc { class BVH; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }

namespace opyn
{
    // generates 3D overlays for the given options and passes them to the
    // output consumer
    void generate_overlay_decorations(
        osc::SceneCache&,
        const OverlayDecorationOptions&,
        const osc::BVH& scene_bvh,
        float fixup_scale_factor,
        const std::function<void(osc::SceneDecoration&&)>& out
    );
}
