#pragma once

#include <OpenSimCreator/ModelGraph/ModelGraph.hpp>

#include <oscar/Utils/UndoRedo.hpp>

namespace osc
{
    using CommittableModelGraph = UndoRedoT<ModelGraph>;
}
