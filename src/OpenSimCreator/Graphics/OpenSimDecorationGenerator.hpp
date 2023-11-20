#pragma once

#include <oscar/Graphics/Mesh.hpp>

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelDisplayHints; }
namespace osc { class OpenSimDecorationOptions; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace SimTK { class State; }

namespace osc
{
    // generates 3D decorations for the given {model, state} pair and passes
    // each of them, tagged with their associated component, to the output
    // consumer
    void GenerateModelDecorations(
        SceneCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSimDecorationOptions const&,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out
    );

    // generates 3D decorations only for `subcomponent` within the given {model, state} pair
    // and passes each of them, tagged with their associated (potentially, sub-subcomponent)
    // component to the output consumer
    void GenerateSubcomponentDecorations(
        SceneCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::Component const& subcomponent,
        OpenSimDecorationOptions const&,
        float fixupScaleFactor,
        std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out,
        bool inclusiveOfProvidedSubcomponent = true
    );

    // tries to convert the given subcomponent mesh into an OSC mesh via the decoration
    // generation API, or throws if it fails in some way
    Mesh ToOscMesh(
        SceneCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::Mesh const&,
        OpenSimDecorationOptions const&,
        float fixupScaleFactor
    );

    // as above, but uncached and defaults decoration options and scale factor
    Mesh ToOscMesh(
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::Mesh const&
    );

    // as above, but also bakes the `OpenSim::Mesh`'s `scale_factors` into the mesh's
    // vertex data
    Mesh ToOscMeshBakeScaleFactors(
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim::Mesh const&
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(
        SceneCache&,
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSimDecorationOptions const&
    );
}
