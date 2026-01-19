#pragma once

#include <liboscar/platform/log_level.h>

#include <functional>
#include <string_view>

namespace opyn
{
    // Sets and overrides the runtime logging callback for log messages emitted by oscar/OPynSim/Simbody/OpenSim
    void set_global_log_sink(std::function<void(osc::LogLevel, std::string_view)>);

    // Globally initializes the opynsim (OpenSim + extensions) API with a default `InitConfiguration`.
    //
    // This should be called by the application before using any `opyn::`, `SimTK::`, or
    // `OpenSim::`-prefixed API. A process may call it multiple times, but only the first
    // call will actually do anything.
    bool init();
}
