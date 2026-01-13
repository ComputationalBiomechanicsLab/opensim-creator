#pragma once

#include <liboscar/graphics/AntiAliasingLevel.h>
#include <liboscar/graphics/scene/SceneCollision.h>
#include <liboscar/graphics/scene/SceneRendererParams.h>
#include <liboscar/maths/Rect.h>
#include <liboscar/maths/Vector2.h>

#include <functional>
#include <optional>
#include <span>

namespace OpenSim { class Component; }
namespace osc { class BVH; }
namespace osc { class IModelStatePair; }
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
        const IModelStatePair&,
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
