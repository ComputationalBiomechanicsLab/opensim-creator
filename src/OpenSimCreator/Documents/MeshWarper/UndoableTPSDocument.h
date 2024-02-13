#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>

#include <oscar/Utils/UndoRedo.h>

namespace osc
{
    using UndoableTPSDocument = UndoRedo<TPSDocument>;
}
