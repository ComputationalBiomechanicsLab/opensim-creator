#pragma once

#include <libOpenSimCreator/Graphics/OpenSimDecorationOptions.h>

#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Mesh.h>

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelDisplayHints; }
namespace osc { class IModelStatePair; }
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
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions&,
        float fixupScaleFactor,
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out
    );

    // as above, but more convenient to use in simple use-cases
    std::vector<SceneDecoration> GenerateModelDecorations(
        SceneCache&,
        const IModelStatePair&,
        const OpenSimDecorationOptions& = {},
        float fixupScaleFactor = 1.0f
    );

    // as above, but more convenient to use in simpler use-cases
    std::vector<SceneDecoration> GenerateModelDecorations(
        SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions& = {},
        float fixupScaleFactor = 1.0f
    );

    // generates 3D decorations only for `subcomponent` within the given {model, state} pair
    // and passes each of them, tagged with their associated (potentially, sub-subcomponent)
    // component to the output consumer
    void GenerateSubcomponentDecorations(
        SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Component& subcomponent,
        const OpenSimDecorationOptions&,
        float fixupScaleFactor,
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out,
        bool inclusiveOfProvidedSubcomponent = true
    );

    // tries to convert the given subcomponent mesh into an OSC mesh via the decoration
    // generation API, or throws if it fails in some way
    Mesh ToOscMesh(
        SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&,
        const OpenSimDecorationOptions&,
        float fixupScaleFactor
    );

    // as above, but uncached and defaults decoration options and scale factor
    Mesh ToOscMesh(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&
    );

    // as above, but also bakes the `OpenSim::Mesh`'s `scale_factors` into the mesh's
    // vertex data
    Mesh ToOscMeshBakeScaleFactors(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&
    );

    // returns the recommended scale factor for the given {model, state} pair
    float GetRecommendedScaleFactor(
        SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions&
    );
}
