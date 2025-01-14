#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/Scene/SceneCollision.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>

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
        Vec2 viewportDims,
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
        Vec2 mouseScreenPos,
        const Rect& viewportScreenRect
    );
}
