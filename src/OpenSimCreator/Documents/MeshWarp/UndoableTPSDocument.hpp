#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocument.hpp>

#include <oscar/Utils/UndoRedo.hpp>

namespace osc
{
    using UndoableTPSDocument = UndoRedoT<TPSDocument>;
}
