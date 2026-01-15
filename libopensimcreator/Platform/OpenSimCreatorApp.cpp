#include "OpenSimCreatorApp.h"

#include <libopensimcreator/UI/OpenSimCreatorTabRegistry.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/tabs/tab_registry.h>
#include <liboscar/utils/Assertions.h>
#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/EnumHelpers.h>
#include <OpenSim/Common/Logger.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <libopynsim/init.h>

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

    // an OpenSim log sink that sinks into OSC's main log
    class OpenSimLogSink final : public OpenSim::LogSink {
        void sinkImpl(const std::string& msg) final
        {
            log_info("%s", msg.c_str());
        }
    };

    void SetupOpenSimLogToUseOSCsLog()
    {
        // disable OpenSim's `opensim.log` default
        //
        // by default, OpenSim creates an `opensim.log` file in the process's working
        // directory. This should be disabled because it screws with running multiple
        // instances of the UI on filesystems that use locking (e.g. Windows) and
        // because it's incredibly obnoxious to have `opensim.log` appear in every
        // working directory from which osc is ran
        log_info("removing OpenSim's default log (opensim.log)");
        OpenSim::Logger::removeFileSink();

        // add OSC in-memory logger
        //
        // this logger collects the logs into a global mutex-protected in-memory structure
        // that the UI can can trivially render (w/o reading files etc.)
        log_info("attaching OpenSim to this log");
        OpenSim::Logger::addSink(std::make_shared<OpenSimLogSink>());
    }

    void GloballySetOpenSimsGeometrySearchPath(const std::filesystem::path& geometryDir)
    {
        // globally set OpenSim's geometry search path
        //
        // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
        // OpenSim implementation will look in these directories for that file

        // TODO: detect and overwrite existing entries?
        log_info("registering OpenSim geometry search path to use osc resources");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometryDir.string());
        log_info("added geometry search path entry: %s", geometryDir.string().c_str());
    }

    bool InitializeOpenSim()
    {
        // globally initialize OpenSim
        log_info("initializing OpenSim (opyn::init)");
        {
            class LogginingInitConfiguration final : public opyn::InitConfiguration {
                void impl_log_message(std::string_view payload, opyn::LogLevel level) final
                {
                    static_assert(num_options<opyn::LogLevel>() == 2);
                    const std::string str{payload};
                    switch (level) {
                    case opyn::LogLevel::info: osc::log_info("%s", str.c_str()); return;
                    case opyn::LogLevel::warn: osc::log_warn("%s", str.c_str()); return;
                    default:                   osc::log_info("%s", str.c_str()); return;
                    }
                }
            };
            LogginingInitConfiguration config;
            opyn::init(config);
        }

        // point OpenSim's log towards OSC's log
        //
        // so that users can see OpenSim log messages in OSC's UI
        SetupOpenSimLogToUseOSCsLog();

        return true;
    }

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
    static const bool s_OpenSimInitialized = InitializeOpenSim();
    return s_OpenSimInitialized;
}

void osc::GloballyAddDirectoryToOpenSimGeometrySearchPath(const std::filesystem::path& p)
{
    GloballySetOpenSimsGeometrySearchPath(p);
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
