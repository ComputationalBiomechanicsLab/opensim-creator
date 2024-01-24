#include "OpenSimCreatorApp.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/StationDefinedFrame.hpp>
#include <OpenSimCreator/UI/OpenSimCreatorTabRegistry.hpp>
#include <OpenSimCreator/OpenSimCreatorConfig.hpp>

#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar_demos/OscarDemosTabRegistry.hpp>
#include <oscar_learnopengl/LearnOpenGLTabRegistry.hpp>
#include <OpenSim/Common/Logger.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>

#include <clocale>
#include <locale>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using osc::fd::StationDefinedFrame;

namespace
{
    // minor alias for setlocale so that any linter complaints about MT unsafety
    // are all deduped to this one source location
    //
    // it's UNSAFE because `setlocale` is a global mutation
    void setLocaleUNSAFE(int category, osc::CStringView locale)
    {
        // disable lint because this function is only called once at application
        // init time
        if (std::setlocale(category, locale.c_str()) == nullptr)  // NOLINT(concurrency-mt-unsafe)
        {
            osc::log::error("error setting locale category %i to %s", category, locale);
        }
    }

    // an OpenSim log sink that sinks into OSC's main log
    class OpenSimLogSink final : public OpenSim::LogSink {
        void sinkImpl(std::string const& msg) final
        {
            osc::log::info("%s", msg.c_str());
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

        osc::log::info("setting locale to US (so that numbers are always in the format '0.x'");
        osc::CStringView const locale = "C";
        osc::SetEnv("LANG", locale, true);
        osc::SetEnv("LC_CTYPE", locale, true);
        osc::SetEnv("LC_NUMERIC", locale, true);
        osc::SetEnv("LC_TIME", locale, true);
        osc::SetEnv("LC_COLLATE", locale, true);
        osc::SetEnv("LC_MONETARY", locale, true);
        osc::SetEnv("LC_MESSAGES", locale, true);
        osc::SetEnv("LC_ALL", locale, true);
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
        osc::log::info("removing OpenSim's default log (opensim.log)");
        OpenSim::Logger::removeFileSink();

        // add OSC in-memory logger
        //
        // this logger collects the logs into a global mutex-protected in-memory structure
        // that the UI can can trivially render (w/o reading files etc.)
        osc::log::info("attaching OpenSim to this log");
        OpenSim::Logger::addSink(std::make_shared<OpenSimLogSink>());
    }

    void RegisterOpenSimTypes()
    {
        osc::log::info("registering OpenSim types");
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();
        RegisterTypes_osimExampleComponents();
        OpenSim::Object::registerType(StationDefinedFrame{});
    }

    void GloballySetOpenSimsGeometrySearchPath(osc::AppConfig const& config)
    {
        // globally set OpenSim's geometry search path
        //
        // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
        // OpenSim implementation will look in these directories for that file
        std::filesystem::path geometryDir = config.getResourceDir() / "geometry";
        osc::log::info("registering OpenSim geometry search path to use osc resources");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometryDir.string());
        osc::log::info("added geometry search path entry: %s", geometryDir.string().c_str());
    }

    bool InitializeOpenSim(osc::AppConfig const& config)
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

        // make OpenSim use OSC's `geometry/` resource directory when searching for
        // geometry files
        GloballySetOpenSimsGeometrySearchPath(config);

        return true;
    }

    // registers user-accessible tabs
    void InitializeTabRegistry(osc::TabRegistry& registry)
    {
        osc::RegisterLearnOpenGLTabs(registry);
        osc::RegisterDemoTabs(registry);
        osc::RegisterOpenSimCreatorTabs(registry);
    }
}


// public API

osc::AppMetadata osc::GetOpenSimCreatorAppMetadata()
{
    return AppMetadata
    {
        OSC_ORGNAME_STRING,
        OSC_APPNAME_STRING,
        OSC_LONG_APPNAME_STRING,
        OSC_VERSION_STRING,
        OSC_BUILD_ID,
        OSC_REPO_URL,
    };
}

osc::AppConfig osc::LoadOpenSimCreatorConfig()
{
    auto metadata = GetOpenSimCreatorAppMetadata();
    return osc::AppConfig{metadata.getOrganizationName(), metadata.getApplicationName()};
}

bool osc::GlobalInitOpenSim()
{
    return GlobalInitOpenSim(LoadOpenSimCreatorConfig());
}

bool osc::GlobalInitOpenSim(AppConfig const& config)
{
    static bool const s_OpenSimInitialized = InitializeOpenSim(config);
    return s_OpenSimInitialized;
}

osc::OpenSimCreatorApp::OpenSimCreatorApp() :
    App{GetOpenSimCreatorAppMetadata()}
{
    GlobalInitOpenSim(getConfig());
    InitializeTabRegistry(*singleton<osc::TabRegistry>());
}
