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

namespace opyn
{
    osc::SceneRendererParams CalcSceneRendererParams(
        const osc::ModelRendererParams&,
        osc::Vector2 viewportDims,
        float viewportDevicePixelRatio,
        osc::AntiAliasingLevel,
        float fixupScaleFactor
    );

    void GenerateDecorations(
        osc::SceneCache&,
        const osc::ModelStatePair&,
        const osc::OpenSimDecorationOptions&,
        const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out
    );

    std::optional<osc::SceneCollision> GetClosestCollision(
        const osc::BVH& sceneBVH,
        osc::SceneCache&,
        std::span<const osc::SceneDecoration> taggedDrawlist,
        const osc::PolarPerspectiveCamera&,
        osc::Vector2 mouseScreenPosition,
        const osc::Rect& viewportScreenRect
    );
}
