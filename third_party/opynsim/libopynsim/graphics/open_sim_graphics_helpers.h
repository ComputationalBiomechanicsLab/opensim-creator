#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>

#include <functional>
#include <optional>
#include <span>

namespace OpenSim { class Component; }
namespace opyn { struct ModelRendererParams; }
namespace opyn { class ModelStatePair; }
namespace opyn { class OpenSimDecorationOptions; }
namespace osc { class BVH; }
namespace osc { class Camera; }
namespace osc { class SceneCache; }
namespace osc { struct SceneDecoration; }

namespace opyn
{
    osc::SceneRendererParams calc_scene_renderer_params(
        const ModelRendererParams&,
        osc::Vector2 viewport_dims,
        float viewport_device_pixel_ratio,
        osc::AntiAliasingLevel,
        float fixup_scale_factor
    );

    void generate_decorations(
        osc::SceneCache&,
        const ModelStatePair&,
        const OpenSimDecorationOptions&,
        const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out
    );

    std::optional<osc::SceneCollision> get_closest_collision(
        const osc::BVH& scene_bvh,
        osc::SceneCache&,
        std::span<const osc::SceneDecoration> tagged_drawlist,
        const osc::Camera&,
        osc::Vector2 mouse_screen_position,
        const osc::Rect& viewport_screen_rect
    );
}
