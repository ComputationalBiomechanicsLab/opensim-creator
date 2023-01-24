#pragma once

#include <functional>

namespace OpenSim { class Component; }
namespace osc { class CustomDecorationOptions; }
namespace osc { class MeshCache; }
namespace osc { class SceneDecoration; }
namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    // generates 3D decorations for the given {model, state, options} tuple and
    // appends them to the output vector
    void GenerateModelDecorations(
        MeshCache&,
        VirtualConstModelStatePair const&,
        CustomDecorationOptions const&,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const&
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(
        MeshCache&,
        VirtualConstModelStatePair const&,
        CustomDecorationOptions const&
    );
}