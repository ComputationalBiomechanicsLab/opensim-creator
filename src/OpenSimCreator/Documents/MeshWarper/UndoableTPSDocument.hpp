#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>

#include <oscar/Utils/UndoRedo.hpp>

namespace osc
{
    using UndoableTPSDocument = UndoRedoT<TPSDocument>;
}
