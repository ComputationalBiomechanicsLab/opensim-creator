#pragma once

#include <functional>

namespace osc { class BVH; }
namespace osc { class MeshCache; }
namespace osc { class OverlayDecorationOptions; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // generates 3D overlays for the given options and passes them to the
    // output consumer
    void GenerateOverlayDecorations(
        MeshCache&,
        OverlayDecorationOptions const&,
        BVH const& sceneBVH,
        std::function<void(SceneDecoration&&)> const& out
    );
}