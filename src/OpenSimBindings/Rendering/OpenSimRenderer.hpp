#pragma once

#include <vector>

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
        std::vector<SceneDecoration>&,
        CustomDecorationOptions const&
    );

    // generates 3D decorations for the given {model, state} pair and appends
    // them to the output vector
    //
    // (options are effectively defaulted)
    void GenerateModelDecorations(
        MeshCache&,
        VirtualConstModelStatePair const&,
        std::vector<SceneDecoration>&
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(MeshCache&, VirtualConstModelStatePair const&);
}