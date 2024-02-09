#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.h>

#include <oscar/Utils/UndoRedo.h>

namespace osc::mi
{
    using UndoableDocument = UndoRedoT<Document>;
}
