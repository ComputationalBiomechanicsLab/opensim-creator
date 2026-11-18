#include "open_sim_creator_app.h"

#include <libopensimcreator/ui/open_sim_creator_tab_registry.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/tabs/tab_registry.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <libopynsim/opynsim.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    OpenSimCreatorApp* g_opensimcreator_app_global = nullptr;

    constexpr auto c_default_panel_states = std::to_array<std::pair<const char*, bool>>({
        {"panels/Actions/enabled", true},

        // Many workflows
        {"panels/Performance/enabled", false},
        {"panels/Log/enabled", true},
        {"panels/Navigator/enabled", true},

        // Model editor OR simulation workflows
        {"panels/Coordinates/enabled", true},
        {"panels/Muscle Plot/enabled", false},
        {"panels/Output Watches/enabled", false},
        {"panels/Output Plots/enabled", false},  // merged with `Output Watches` around v0.5.15
        {"panels/Properties/enabled", true},
        {"panels/Selection Details/enabled", true},

        // Simulation workflow
        {"panels/Simulation Details/enabled", false},  // replaced by `Properties` around v0.5.15

        // Mesh warper workflow
        {"panels/Source Mesh/enabled", true},
        {"panels/Destination Mesh/enabled", true},
        {"panels/Result/enabled", true},

        // Model warper workflow
        {"panels/Control Panel/enabled", true},
        {"panels/Source Model/enabled", true},
        {"panels/Result Model/enabled", true},
    });

    void initialize_open_sim_creator_specific_setting_defaults(AppSettings& settings)
    {
        for (const auto& [setting_id, default_state] : c_default_panel_states) {
            settings.set_value_if_not_found(setting_id, default_state, AppSettingScope::System);
        }
        settings.set_value_if_not_found(
            "graphics/render_scale", Variant{1.0f}, AppSettingScope::System);
    }
}

bool osc::globally_init_open_sim()
{
    static const bool s_open_sim_initialized = [] {
        // The desktop app logs a little bit more by default
        opyn::set_log_level(osc::LogLevel::info);
        return opyn::init();
    }();
    return s_open_sim_initialized;
}

void osc::globally_add_directory_to_open_sim_geometry_search_path(const std::filesystem::path& p)
{
    if (std::filesystem::exists(p) and std::filesystem::is_directory(p)) {
        // Always append to the _end_ of the geometry search path, so that the
        // existing paths (e.g. local to the model file) take precedence (#51).
        opyn::append_search_path(p);
        log_info("added geometry search path entry: {}", p.string());
    } else {
        log_warn(
            "{}: does not exist, it will not be added to the geometry search path", p.string());
    }
}

const OpenSimCreatorApp& osc::OpenSimCreatorApp::get()
{
    OSC_ASSERT(
        g_opensimcreator_app_global &&
        "OpenSimCreatorApp is not initialized: have you constructed a (singleton) instance of OpenSimCreatorApp?");
    return *g_opensimcreator_app_global;
}

osc::OpenSimCreatorApp::OpenSimCreatorApp() :
    OpenSimCreatorApp{AppMetadata{}}
{}

osc::OpenSimCreatorApp::OpenSimCreatorApp(const AppMetadata& metadata) :
    App{metadata}
{
    globally_init_open_sim();
    const ResourcePath geometry_directory_path{"OpenSimCreator/geometry"};
    if (const auto geometry_directory = resource_filepath(geometry_directory_path)) {
        globally_add_directory_to_open_sim_geometry_search_path(*geometry_directory);
    } else {
        log_error(
            "{}: cannot find geometry directory resource: falling back to not using one at all. You might need to update the osc.toml configuration file.",
            geometry_directory_path.string());
    }
    register_open_sim_creator_tabs(upd_tab_registry());
    initialize_open_sim_creator_specific_setting_defaults(upd_settings());
    g_opensimcreator_app_global = this;
}

osc::OpenSimCreatorApp::~OpenSimCreatorApp() noexcept
{
    g_opensimcreator_app_global = nullptr;
}

TabRegistry& osc::OpenSimCreatorApp::upd_tab_registry()
{
    return *singleton<TabRegistry>();
}
