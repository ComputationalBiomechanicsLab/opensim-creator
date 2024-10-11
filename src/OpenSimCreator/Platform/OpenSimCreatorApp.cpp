#include "OpenSimCreatorApp.h"

#include <OpenSimCreator/Documents/CustomComponents/CrossProductEdge.h>
#include <OpenSimCreator/Documents/CustomComponents/MidpointLandmark.h>
#include <OpenSimCreator/Documents/CustomComponents/PointToPointEdge.h>
#include <OpenSimCreator/Documents/CustomComponents/SphereLandmark.h>
#include <OpenSimCreator/UI/OpenSimCreatorTabRegistry.h>

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/Logger.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>
#include <OpenSimThirdPartyPlugins/RegisterTypes_osimPlugin.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Tabs/TabRegistry.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/CStringView.h>
#include <oscar_demos/OscarDemosTabRegistry.h>
#include <oscar_simbody/UI/OscarSimbodyTabRegistry.h>

#include <array>
#include <clocale>
#include <filesystem>
#include <locale>
#include <memory>
#include <string>
#include <utility>

using namespace osc::fd;
using namespace osc;

namespace
{
    OpenSimCreatorApp* g_opensimcreator_app_global = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    constexpr auto c_default_panel_states = std::to_array<std::pair<const char*, bool>>({
        {"panels/Actions/enabled", true},
        {"panels/Navigator/enabled", true},
        {"panels/Log/enabled", true},
        {"panels/Properties/enabled", true},
        {"panels/Selection Details/enabled", true},
        {"panels/Simulation Details/enabled", false},  // replaced by `Properties` around v0.5.15
        {"panels/Coordinates/enabled", true},
        {"panels/Performance/enabled", false},
        {"panels/Muscle Plot/enabled", false},
        {"panels/Output Watches/enabled", false},
        {"panels/Output Plots/enabled", false},        // merged with `Output Watches` around v0.5.15
        {"panels/Source Mesh/enabled", true},
        {"panels/Destination Mesh/enabled", true},
        {"panels/Result/enabled", true},
    });

    // minor alias for setlocale so that any linter complaints about MT unsafety
    // are all deduped to this one source location
    //
    // it's UNSAFE because `setlocale` is a global mutation
    void setLocaleUNSAFE(int category, CStringView locale)
    {
        // disable lint because this function is only called once at application
        // init time
        if (std::setlocale(category, locale.c_str()) == nullptr)  // NOLINT(concurrency-mt-unsafe)
        {
            log_error("error setting locale category %i to %s", category, locale);
        }
    }

    // an OpenSim log sink that sinks into OSC's main log
    class OpenSimLogSink final : public OpenSim::LogSink {
        void sinkImpl(const std::string& msg) final
        {
            log_info("%s", msg.c_str());
        }
    };

    void SetGlobalLocaleToMatchOpenSim()
    {
        // these are because OpenSim is inconsistient about handling locales
        //
        // it *writes* OSIM files using the locale, so you can end up with entries like:
        //
        //     <PathPoint_X>0,1323</PathPoint_X>
        //
        // but it *reads* OSIM files with the assumption that numbers will be in the format 'x.y'

        log_info("setting locale to US (so that numbers are always in the format '0.x'");
        const CStringView locale = "C";
        set_environment_variable("LANG", locale, true);
        set_environment_variable("LC_CTYPE", locale, true);
        set_environment_variable("LC_NUMERIC", locale, true);
        set_environment_variable("LC_TIME", locale, true);
        set_environment_variable("LC_COLLATE", locale, true);
        set_environment_variable("LC_MONETARY", locale, true);
        set_environment_variable("LC_MESSAGES", locale, true);
        set_environment_variable("LC_ALL", locale, true);
#ifdef LC_CTYPE
        setLocaleUNSAFE(LC_CTYPE, locale);
#endif
#ifdef LC_NUMERIC
        setLocaleUNSAFE(LC_NUMERIC, locale);
#endif
#ifdef LC_TIME
        setLocaleUNSAFE(LC_TIME, locale);
#endif
#ifdef LC_COLLATE
        setLocaleUNSAFE(LC_COLLATE, locale);
#endif
#ifdef LC_MONETARY
        setLocaleUNSAFE(LC_MONETARY, locale);
#endif
#ifdef LC_MESSAGES
        setLocaleUNSAFE(LC_MESSAGES, locale);
#endif
#ifdef LC_ALL
        setLocaleUNSAFE(LC_ALL, locale);
#endif
        std::locale::global(std::locale{locale.c_str()});
    }

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

    void RegisterOpenSimTypes()
    {
        log_info("registering OpenSim types");
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();
        RegisterTypes_osimExampleComponents();
        RegisterTypes_osimPlugin();  // from `OpenSimThirdPartyPlugins`

        // custom components
        OpenSim::Object::registerType(CrossProductEdge{});
        OpenSim::Object::registerType(MidpointLandmark{});
        OpenSim::Object::registerType(PointToPointEdge{});
        OpenSim::Object::registerType(SphereLandmark{});
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
        // make this process (OSC) globally use the same locale that OpenSim uses
        //
        // this is necessary because OpenSim assumes a certain locale (see function
        // impl. for more details)
        SetGlobalLocaleToMatchOpenSim();

        // point OpenSim's log towards OSC's log
        //
        // so that users can see OpenSim log messages in OSC's UI
        SetupOpenSimLogToUseOSCsLog();

        // explicitly load OpenSim libs
        //
        // this is necessary because some compilers will refuse to link a library
        // unless symbols from that library are directly used.
        //
        // Unfortunately, OpenSim relies on weak linkage *and* static library-loading
        // side-effects. This means that (e.g.) the loading of muscles into the runtime
        // happens in a static initializer *in the library*.
        //
        // osc may not link that library, though, because the source code in OSC may
        // not *directly* use a symbol exported by the library (e.g. the code might use
        // OpenSim::Muscle references, but not actually concretely refer to a muscle
        // implementation method (e.g. a ctor)
        RegisterOpenSimTypes();

        return true;
    }

    // registers user-accessible tabs
    void InitializeTabRegistry(TabRegistry& registry)
    {
        register_demo_tabs(registry);
        RegisterOpenSimCreatorTabs(registry);
        RegisterOscarSimbodyTabs(registry);
    }

    void InitializeOpenSimCreatorSpecificSettingDefaults(AppSettings& settings)
    {
        for (const auto& [setting_id, default_state] : c_default_panel_states) {
            settings.set_value(setting_id, default_state, AppSettingScope::System);
        }
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
    GloballyAddDirectoryToOpenSimGeometrySearchPath(resource_filepath("geometry"));
    InitializeTabRegistry(*singleton<TabRegistry>());
    InitializeOpenSimCreatorSpecificSettingDefaults(upd_settings());
    g_opensimcreator_app_global = this;
}

osc::OpenSimCreatorApp::~OpenSimCreatorApp() noexcept
{
    g_opensimcreator_app_global = nullptr;
}

std::string osc::OpenSimCreatorApp::docs_url() const
{
    if (const auto runtime_url = settings().find_value("docs_url")) {
        return to<std::string>(*runtime_url);
    }
    else {
        return "https://docs.opensimcreator.com";
    }
}
