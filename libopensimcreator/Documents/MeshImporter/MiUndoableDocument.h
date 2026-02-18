#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiDocument.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc
{
    using MiUndoableDocument = UndoRedo<MiDocument>;
}
