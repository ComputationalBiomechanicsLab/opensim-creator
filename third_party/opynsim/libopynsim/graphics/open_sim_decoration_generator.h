#pragma once

#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/mesh.h>

#include <functional>

namespace OpenSim { class Component; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class ModelDisplayHints; }
namespace opyn { class ModelStatePair; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace SimTK { class State; }

namespace opyn
{
    // generates 3D decorations for the given {model, state} pair and passes
    // each of them, tagged with their associated component, to the output
    // consumer
    void generate_model_decorations(
        osc::SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions&,
        float fixup_scale_factor,
        const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out
    );

    // as above, but more convenient to use in simple use-cases
    std::vector<osc::SceneDecoration> generate_model_decorations(
        osc::SceneCache&,
        const ModelStatePair&,
        const OpenSimDecorationOptions& = {},
        float fixup_scale_factor = 1.0f
    );

    // as above, but more convenient to use in simpler use-cases
    std::vector<osc::SceneDecoration> generate_model_decorations(
        osc::SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions& = {},
        float fixup_scale_factor = 1.0f
    );

    // generates 3D decorations only for `subcomponent` within the given {model, state} pair
    // and passes each of them, tagged with their associated (potentially, sub-subcomponent)
    // component to the output consumer
    void generate_subcomponent_decorations(
        osc::SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Component& subcomponent,
        const OpenSimDecorationOptions&,
        float fixup_scale_factor,
        const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out,
        bool inclusive_of_provided_subcomponent = true
    );

    // tries to convert the given subcomponent mesh into an OSC mesh via the decoration
    // generation API, or throws if it fails in some way
    osc::Mesh to_osc_mesh(
        osc::SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&,
        const OpenSimDecorationOptions&,
        float fixup_scale_factor
    );

    // as above, but uncached and defaults decoration options and scale factor
    osc::Mesh to_osc_mesh(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&
    );

    // as above, but also bakes the `OpenSim::Mesh`'s `scale_factors` into the mesh's
    // vertex data
    osc::Mesh to_osc_mesh_bake_scale_factors(
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSim::Mesh&
    );

    // returns the recommended scale factor for the given {model, state} pair
    float get_recommended_scale_factor(
        osc::SceneCache&,
        const OpenSim::Model&,
        const SimTK::State&,
        const OpenSimDecorationOptions&
    );
}
