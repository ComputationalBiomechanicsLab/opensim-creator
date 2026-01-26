#pragma once

#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/mesh.h>

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelDisplayHints; }
namespace osc { class ModelStatePair; }
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
        const opyn::OpenSimDecorationOptions&,
        float fixupScaleFactor,
        const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out
    );

    // as above, but more convenient to use in simple use-cases
    std::vector<SceneDecoration> GenerateModelDecorations(
        SceneCache&,
        const ModelStatePair&,
        const opyn::OpenSimDecorationOptions& = {},
        float fixupScaleFactor = 1.0f
    );

    // as above, but more convenient to use in simpler use-cases
    std::vector<SceneDecoration> GenerateModelDecorations(
        SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const opyn::OpenSimDecorationOptions& = {},
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
        const opyn::OpenSimDecorationOptions&,
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
        const opyn::OpenSimDecorationOptions&,
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
        const opyn::OpenSimDecorationOptions&
    );
}
