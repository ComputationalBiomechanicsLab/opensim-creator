#include "model_drawlist_generator.hpp"

#include "src/3d/untextured_vert.hpp"
#include "src/opensim_bindings/lambda_geometry_visitor.hpp"
#include "src/opensim_bindings/model_drawlist.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Orientation.h>
#include <SimTKsimbody.h>

#include <vector>

void osmv::generate_decoration_drawlist(
    OpenSim::Component const& root,
    SimTK::State const& state,
    OpenSim::ModelDisplayHints const& hints,
    Gpu_cache& gpu_cache,
    Model_drawlist& drawlist) {

    std::vector<Untextured_vert> vert_swap_space;
    OpenSim::Component const* current_component = nullptr;
    SimTK::SimbodyMatterSubsystem const& matter = root.getSystem().getMatterSubsystem();

    // called whenever the backend emits geometry
    auto on_instance_created = [&current_component, &drawlist](Mesh_instance const& mi) {
        drawlist.emplace_back(current_component, mi);
    };

    auto visitor = Lambda_geometry_visitor{std::move(on_instance_created), vert_swap_space, gpu_cache, matter, state};

    SimTK::Array_<SimTK::DecorativeGeometry> dg;
    for (OpenSim::Component const& c : root.getComponentList()) {
        current_component = &c;

        dg.clear();
        c.generateDecorations(true, hints, state, dg);
        for (SimTK::DecorativeGeometry const& geom : dg) {
            geom.implementGeometry(visitor);
        }

        dg.clear();
        c.generateDecorations(false, hints, state, dg);
        for (SimTK::DecorativeGeometry const& geom : dg) {
            geom.implementGeometry(visitor);
        }
    }
}
