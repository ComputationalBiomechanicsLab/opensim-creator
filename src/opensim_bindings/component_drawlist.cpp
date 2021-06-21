#include "component_drawlist.hpp"

#include "src/3d/3d.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/opensim_bindings/component_drawlist.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <SimTKcommon.h>
#include <SimTKsimbody.h>

#include <utility>
#include <vector>

namespace osc {
    struct Mesh_instance;
}

void osc::generate_component_decorations(
    OpenSim::Component const& root,
    SimTK::State const& state,
    OpenSim::ModelDisplayHints const& hints,
    GPU_storage& gpu_cache,
    Component_drawlist& drawlist,
    ModelDrawlistFlags flags) {

    Untextured_mesh mesh_swap;
    OpenSim::Component const* current_component = nullptr;
    SimTK::SimbodyMatterSubsystem const& matter = root.getSystem().getMatterSubsystem();

    // called whenever the backend emits geometry
    auto on_instance_created = [&current_component, &drawlist](Mesh_instance const& mi) {
        drawlist.push_back(current_component, mi);
    };

    auto visitor = Lambda_geometry_visitor{std::move(on_instance_created), mesh_swap, gpu_cache, matter, state};

    SimTK::Array_<SimTK::DecorativeGeometry> dg;
    for (OpenSim::Component const& c : root.getComponentList()) {
        current_component = &c;

        if (flags & ModelDrawlistFlags_StaticGeometry) {
            c.generateDecorations(true, hints, state, dg);
            for (SimTK::DecorativeGeometry const& geom : dg) {
                geom.implementGeometry(visitor);
            }
            dg.clear();
        }

        if (flags & ModelDrawlistFlags_DynamicGeometry) {
            c.generateDecorations(false, hints, state, dg);
            for (SimTK::DecorativeGeometry const& geom : dg) {
                geom.implementGeometry(visitor);
            }
            dg.clear();
        }
    }
}
