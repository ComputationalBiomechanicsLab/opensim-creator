#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>

#include <liboscar/utils/UndoRedo.h>

namespace osc
{
    using UndoableTPSDocument = UndoRedo<TPSDocument>;
}
