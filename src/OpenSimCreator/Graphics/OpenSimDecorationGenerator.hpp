#pragma once

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { class MeshCache; }
namespace osc { struct SceneDecoration; }
namespace SimTK { class State; }

namespace osc
{
    // generates 3D decorations for the given {model, state} pair and passes
    // each of them, tagged with their associated component, to the output
    // consumer
    void GenerateModelDecorations(
        MeshCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSimDecorationOptions const&,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(
        MeshCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSimDecorationOptions const&
    );
}
