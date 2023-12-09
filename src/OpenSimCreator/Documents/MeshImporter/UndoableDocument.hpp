#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.hpp>

#include <oscar/Utils/UndoRedo.hpp>

namespace osc::mi
{
    using UndoableDocument = UndoRedoT<Document>;
}
