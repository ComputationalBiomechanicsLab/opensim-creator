#include "in_memory_mesh.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <OpenSim/Simulation/Model/Frame.h>

using namespace opyn;

void opyn::InMemoryMesh::impl_generate_custom_decorations(
    const SimTK::State& state,
    const std::function<void(osc::SceneDecoration&&)>& out) const
{
    out(osc::SceneDecoration{
        .mesh      = osc_mesh_,
        .transform = osc::to<osc::Transform>(getFrame().getTransformInGround(state)),
        .shading   = to_color(get_Appearance()),
    });
}
