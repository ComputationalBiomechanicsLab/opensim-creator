#pragma once

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/AppMetadata.h>

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

    // an `App` that also calls `GlobalInitOpenSim`
    class OpenSimCreatorApp final : private App {
    public:
        OpenSimCreatorApp();

        using App::show;
    };
}
