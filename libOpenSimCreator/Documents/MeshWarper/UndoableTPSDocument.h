#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>

#include <liboscar/Utils/UndoRedo.h>

namespace osc
{
    using UndoableTPSDocument = UndoRedo<TPSDocument>;
}
