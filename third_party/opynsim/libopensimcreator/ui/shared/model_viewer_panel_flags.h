#pragma once

#include <liboscar/utilities/flags.h>

namespace osc
{
    enum class ModelViewerPanelFlag : unsigned {
        None        = 0,
        NoHittest   = 1<<0,
        NUM_OPTIONS =    1,
    };
    using ModelViewerPanelFlags = Flags<ModelViewerPanelFlag>;
}
