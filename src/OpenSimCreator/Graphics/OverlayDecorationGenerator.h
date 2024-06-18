#pragma once

#include <functional>

namespace osc { class BVH; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }

namespace osc
{
    // generates 3D overlays for the given options and passes them to the
    // output consumer
    void GenerateOverlayDecorations(
        SceneCache&,
        const OverlayDecorationOptions&,
        const BVH& sceneBVH,
        std::function<void(SceneDecoration&&)> const& out
    );
}
