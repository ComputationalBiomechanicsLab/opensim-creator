#pragma once

#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Maths/Rect.hpp>

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <functional>
#include <optional>

namespace OpenSim { class Component; }
namespace osc { struct BVH; }
namespace osc { class MeshCache; }
namespace osc { struct ModelRendererParams; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct SceneDecoration; }
namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    SceneRendererParams CalcSceneRendererParams(
        ModelRendererParams const&,
        glm::vec2 viewportDims,
        int32_t samples,
        float fixupScaleFactor
    );

    void GenerateDecorations(
        MeshCache&,
        VirtualConstModelStatePair const&,
        OpenSimDecorationOptions const&,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out
    );

    std::optional<SceneCollision> GetClosestCollision(
        BVH const& sceneBVH,
        nonstd::span<SceneDecoration const> taggedDrawlist,
        PolarPerspectiveCamera const&,
        glm::vec2 mouseScreenPos,
        Rect const& viewportScreenRect
    );
}