#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc
{
    using UndoableTPSDocument = UndoRedo<TPSDocument>;
}
