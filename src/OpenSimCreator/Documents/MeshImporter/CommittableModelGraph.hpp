#pragma once

#include <OpenSimCreator/Documents/MeshImporter/ModelGraph.hpp>

#include <oscar/Utils/UndoRedo.hpp>

namespace osc
{
    using CommittableModelGraph = UndoRedoT<ModelGraph>;
}
