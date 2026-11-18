#pragma once

#include <liboscar/platform/app.h>

#include <filesystem>

namespace osc
{
    class AppMetdata;
    class TabRegistry;

    // manually ensure OpenSim is initialized
    //
    // e.g. initializes OpenSim logging, registering components, etc.
    bool globally_init_open_sim();

    // adds the given filesystem path to a directory to OpenSim's global
    // search list that it uses when searching for mesh files
    void globally_add_directory_to_open_sim_geometry_search_path(const std::filesystem::path&);

    // an `App` that:
    //
    // - ensures `globally_init_open_sim` has been called
    // - ensures `resources/geometry` has been added to the geometry search path
    // - initializes a `TabRegistry` singleton instance containing all user-facing tabs
    // - initializes any other OpenSim-Creator-specific settings
    class OpenSimCreatorApp final : public App {
    public:
        // returns the currently-active application global
        static const OpenSimCreatorApp& get();

        OpenSimCreatorApp();
        explicit OpenSimCreatorApp(const AppMetadata&);
        OpenSimCreatorApp(const OpenSimCreatorApp&) = delete;
        OpenSimCreatorApp(OpenSimCreatorApp&&) noexcept = delete;
        OpenSimCreatorApp& operator=(const OpenSimCreatorApp&) = delete;
        OpenSimCreatorApp& operator=(OpenSimCreatorApp&&) noexcept = delete;
        ~OpenSimCreatorApp() noexcept;

        using App::show;

        TabRegistry& upd_tab_registry();
    };
}
