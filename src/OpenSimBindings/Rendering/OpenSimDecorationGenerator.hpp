#pragma once

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace osc { class CustomDecorationOptions; }
namespace osc { class MeshCache; }
namespace osc { class SceneDecoration; }
namespace SimTK { class State; }

namespace osc
{
    // generates 3D decorations for the given {model, state, options} tuple and
    // appends them to the output vector
    void GenerateModelDecorations(
        MeshCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        CustomDecorationOptions const&,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(
        MeshCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        CustomDecorationOptions const&,
        float fixupScaleFactor
    );
}