#pragma once

#include <functional>

namespace osc { struct BVH; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class MeshCache; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    // generates 3D overlays for the given options and passes them to the
    // output consumer
	void GenerateOverlayDecorations(
        MeshCache&,
        CustomRenderingOptions const&,
        BVH const& sceneBVH,
        std::function<void(SceneDecoration&&)> const& out
	);
}