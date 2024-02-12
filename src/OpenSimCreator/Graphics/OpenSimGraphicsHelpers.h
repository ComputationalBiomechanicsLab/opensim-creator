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
        ModelRendererParams const&,
        Vec2 viewportDims,
        AntiAliasingLevel,
        float fixupScaleFactor
    );

    void GenerateDecorations(
        SceneCache&,
        IConstModelStatePair const&,
        OpenSimDecorationOptions const&,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out
    );

    std::optional<SceneCollision> GetClosestCollision(
        BVH const& sceneBVH,
        SceneCache&,
        std::span<SceneDecoration const> taggedDrawlist,
        PolarPerspectiveCamera const&,
        Vec2 mouseScreenPos,
        Rect const& viewportScreenRect
    );
}
