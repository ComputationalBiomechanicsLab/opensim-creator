#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>

#include <functional>
#include <optional>
#include <span>

namespace OpenSim { class Component; }
namespace osc { class BVH; }
namespace osc { struct ModelRendererParams; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { class IConstModelStatePair; }

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
        const IConstModelStatePair&,
        const OpenSimDecorationOptions&,
        std::function<void(const OpenSim::Component&, SceneDecoration&&)> const& out
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
