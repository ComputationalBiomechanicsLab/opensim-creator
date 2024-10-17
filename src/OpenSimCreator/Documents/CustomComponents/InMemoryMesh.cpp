#include "InMemoryMesh.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKConverters.h>

#include <OpenSim/Simulation/Model/Frame.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>

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
