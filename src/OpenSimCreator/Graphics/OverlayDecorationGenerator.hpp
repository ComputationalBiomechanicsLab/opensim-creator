#pragma once

#include <functional>

namespace osc { class BVH; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneMeshCache; }

namespace osc
{
    // generates 3D overlays for the given options and passes them to the
    // output consumer
    void GenerateOverlayDecorations(
        SceneMeshCache&,
        OverlayDecorationOptions const&,
        BVH const& sceneBVH,
        std::function<void(SceneDecoration&&)> const& out
    );
}
