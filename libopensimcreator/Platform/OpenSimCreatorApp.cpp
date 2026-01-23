#include "OpenSimCreatorApp.h"

#include <libopensimcreator/UI/OpenSimCreatorTabRegistry.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/tabs/tab_registry.h>
#include <liboscar/utils/assertions.h>
#include <liboscar/utils/c_string_view.h>
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
    OpenSimCreatorApp* g_opensimcreator_app_global = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
        {"panels/Output Plots/enabled", false},        // merged with `Output Watches` around v0.5.15
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

    void InitializeOpenSimCreatorSpecificSettingDefaults(AppSettings& settings)
    {
        for (const auto& [setting_id, default_state] : c_default_panel_states) {
            settings.set_value_if_not_found(setting_id, default_state, AppSettingScope::System);
        }
        settings.set_value_if_not_found("graphics/render_scale", Variant{1.0f}, AppSettingScope::System);
    }
}

bool osc::GloballyInitOpenSim()
{
    static const bool s_OpenSimInitialized = []{
        opyn::set_log_level(osc::LogLevel::info);  // The desktop app logs a little bit more by default
        return opyn::init();
    }();
    return s_OpenSimInitialized;
}

void osc::GloballyAddDirectoryToOpenSimGeometrySearchPath(const std::filesystem::path& p)
{
    opyn::add_geometry_directory(p);
}

const OpenSimCreatorApp& osc::OpenSimCreatorApp::get()
{
    OSC_ASSERT(g_opensimcreator_app_global && "OpenSimCreatorApp is not initialized: have you constructed a (singleton) instance of OpenSimCreatorApp?");
    return *g_opensimcreator_app_global;
}

osc::OpenSimCreatorApp::OpenSimCreatorApp() :
    OpenSimCreatorApp{AppMetadata{}}
{}

osc::OpenSimCreatorApp::OpenSimCreatorApp(const AppMetadata& metadata) :
    App{metadata}
{
    GloballyInitOpenSim();
    const ResourcePath geometryDirectoryPath{"OpenSimCreator/geometry"};
    if (const auto geometryDirectory = resource_filepath(geometryDirectoryPath)) {
        GloballyAddDirectoryToOpenSimGeometrySearchPath(*geometryDirectory);
    } else {
        log_error("%s: cannot find geometry directory resource: falling back to not using one at all. You might need to update the osc.toml configuration file.", geometryDirectoryPath.string().c_str());
    }
    RegisterOpenSimCreatorTabs(upd_tab_registry());
    InitializeOpenSimCreatorSpecificSettingDefaults(upd_settings());
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
