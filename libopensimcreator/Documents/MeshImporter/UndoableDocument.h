#pragma once

#include <libopensimcreator/Documents/MeshImporter/Document.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc::mi
{
    using UndoableDocument = UndoRedo<Document>;
}
