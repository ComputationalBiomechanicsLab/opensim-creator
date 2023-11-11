#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

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
namespace osc { class VirtualConstModelStatePair; }

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
        VirtualConstModelStatePair const&,
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
