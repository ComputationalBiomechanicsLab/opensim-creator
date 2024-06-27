#pragma once

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/AppMetadata.h>

#include <filesystem>

namespace osc { class AppSettings; }

namespace osc
{
    AppMetadata GetOpenSimCreatorAppMetadata();
    AppSettings LoadOpenSimCreatorSettings();

    // manually ensure OpenSim is initialized
    //
    // e.g. initializes OpenSim logging, registering components, etc.
    bool GlobalInitOpenSim();

    // adds the given filesystem path to a directory to OpenSim's global
    // search list that it uses when searching for mesh files
    void AddDirectoryToOpenSimGeometrySearchPath(const std::filesystem::path&);

    // an `App` that:
    //
    // - ensures `GlobalInitOpenSim` has been called
    // - ensures `resources/geometry` has been added to the geometry search path
    // - initializes a `TabRegistry` singleton instance containing all user-facing tabs
    // - initializes any other OpenSim-Creator-specific settings
    class OpenSimCreatorApp final : public App {
    public:
        // returns the currently-active application global
        static const OpenSimCreatorApp& get();

        OpenSimCreatorApp();
        OpenSimCreatorApp(const OpenSimCreatorApp&) = delete;
        OpenSimCreatorApp(OpenSimCreatorApp&&) noexcept = delete;
        OpenSimCreatorApp& operator=(const OpenSimCreatorApp&) = delete;
        OpenSimCreatorApp& operator=(OpenSimCreatorApp&&) noexcept = delete;
        ~OpenSimCreatorApp() noexcept;

        using App::show;

        std::string docs_url() const;
    };
}
