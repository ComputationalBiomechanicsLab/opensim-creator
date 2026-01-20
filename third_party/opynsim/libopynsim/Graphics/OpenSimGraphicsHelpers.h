#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector2.h>

#include <functional>
#include <optional>
#include <span>

namespace OpenSim { class Component; }
namespace osc { class BVH; }
namespace osc { class ModelStatePair; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { class SceneCache; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    SceneRendererParams CalcSceneRendererParams(
        const ModelRendererParams&,
        Vector2 viewportDims,
        float viewportDevicePixelRatio,
        AntiAliasingLevel,
        float fixupScaleFactor
    );

    void GenerateDecorations(
        SceneCache&,
        const ModelStatePair&,
        const OpenSimDecorationOptions&,
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out
    );

    std::optional<SceneCollision> GetClosestCollision(
        const BVH& sceneBVH,
        SceneCache&,
        std::span<const SceneDecoration> taggedDrawlist,
        const PolarPerspectiveCamera&,
        Vector2 mouseScreenPosition,
        const Rect& viewportScreenRect
    );
}
