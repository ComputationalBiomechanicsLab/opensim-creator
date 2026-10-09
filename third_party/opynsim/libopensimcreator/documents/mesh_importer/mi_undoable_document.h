#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_document.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc
{
    using MiUndoableDocument = UndoRedo<MiDocument>;
}
