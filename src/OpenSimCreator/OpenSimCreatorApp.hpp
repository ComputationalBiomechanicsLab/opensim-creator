#pragma once

#include <oscar/Platform/App.hpp>

namespace osc { class AppConfig; }

namespace osc
{
    // manually ensure OpenSim is initialized
    //
    // e.g. initializes OpenSim logging, registering components, etc.
    bool GlobalInitOpenSim(AppConfig const&);

    // an `osc::App` that also calls `GlobalInitOpenSim`
    class OpenSimCreatorApp final : public App {
    public:
        OpenSimCreatorApp();
    };
}
