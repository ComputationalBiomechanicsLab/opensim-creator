#pragma once

#include "src/Platform/App.hpp"

namespace osc { class Config; }

namespace osc
{
    // manually ensure OpenSim is initialized
    //
    // e.g. initializes OpenSim logging, registering components, etc.
    bool GlobalInitOpenSim(Config const&);

    // an `osc::App` that also calls `GlobalInitOpenSim`
    class OpenSimApp final : public App {
    public:
        OpenSimApp();
    };
}