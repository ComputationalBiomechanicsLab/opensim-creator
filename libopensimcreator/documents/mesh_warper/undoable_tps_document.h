#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document.h>

#include <liboscar/utilities/undo_redo.h>

namespace osc
{
    using UndoableTPSDocument = UndoRedo<TPSDocument>;
}
