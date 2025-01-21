#pragma once

#include <libopensimcreator/Documents/MeshImporter/Document.h>

#include <liboscar/Utils/UndoRedo.h>

namespace osc::mi
{
    using UndoableDocument = UndoRedo<Document>;
}
