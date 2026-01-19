#include "InMemoryMesh.h"

#include <libopynsim/Utils/OpenSimHelpers.h>
#include <libopynsim/Utils/simbody_x_oscar.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <OpenSim/Simulation/Model/Frame.h>

void osc::mow::InMemoryMesh::implGenerateCustomDecorations(
    const SimTK::State& state,
    const std::function<void(SceneDecoration&&)>& out) const
{
    out(SceneDecoration{
        .mesh = m_OscMesh,
        .transform = to<Transform>(getFrame().getTransformInGround(state)),
        .shading = to_color(get_Appearance()),
    });
}
