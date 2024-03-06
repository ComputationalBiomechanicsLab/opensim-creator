#include "InMemoryMesh.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <OpenSim/Simulation/Model/Frame.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>

void osc::InMemoryMesh::implGenerateCustomDecorations(
    SimTK::State const& state,
    std::function<void(SceneDecoration&&)> const& out) const
{
    out({
        .mesh = m_OscMesh,
        .transform = decompose_to_transform(getFrame().getTransformInGround(state)),
        .color = ToColor(get_Appearance()),
    });
}
