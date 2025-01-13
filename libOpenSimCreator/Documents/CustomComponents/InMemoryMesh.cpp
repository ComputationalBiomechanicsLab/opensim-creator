#include "InMemoryMesh.h"

#include <libOpenSimCreator/Utils/OpenSimHelpers.h>
#include <libOpenSimCreator/Utils/SimTKConverters.h>

#include <liboscar/Graphics/Scene/SceneDecoration.h>
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
