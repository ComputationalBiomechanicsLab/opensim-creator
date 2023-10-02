#pragma once

#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppMetadata.hpp>

namespace osc { class AppConfig; }

namespace osc
{
    AppMetadata GetOpenSimCreatorAppMetadata();
    AppConfig LoadOpenSimCreatorConfig();

    // manually ensure OpenSim is initialized
    //
    // e.g. initializes OpenSim logging, registering components, etc.
    bool GlobalInitOpenSim();
    bool GlobalInitOpenSim(AppConfig const&);

    // an `osc::App` that also calls `GlobalInitOpenSim`
    class OpenSimCreatorApp final : public App {
    public:
        OpenSimCreatorApp();
    };
}
