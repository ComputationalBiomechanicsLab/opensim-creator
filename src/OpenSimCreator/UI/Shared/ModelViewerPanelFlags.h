#pragma once

#include <oscar/Utils/Flags.h>

namespace osc
{
    enum class ModelViewerPanelFlag : unsigned {
        None        = 0,
        NoHittest   = 1<<0,
        NUM_OPTIONS =    1,
    };
    using ModelViewerPanelFlags = Flags<ModelViewerPanelFlag>;
}
