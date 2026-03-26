#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc
{
    /// Represents a `MwDocument` with commit-based undo/redo behavior.
    using MwUndoableDocument = UndoRedo<MwDocument>;
}
