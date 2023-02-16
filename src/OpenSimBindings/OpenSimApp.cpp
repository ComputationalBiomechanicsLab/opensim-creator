#include "OpenSimApp.hpp"

#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Tabs/TabRegistry.hpp"
#include "src/Tabs/TabRegistryEntry.hpp"
#include "src/Utils/CStringView.hpp"

// registered tabs
#include "src/Tabs/Experiments/CustomWidgetsTab.hpp"
#include "src/Tabs/Experiments/HittestTab.hpp"
#include "src/Tabs/Experiments/ImGuiDemoTab.hpp"
#include "src/Tabs/Experiments/ImGuizmoDemoTab.hpp"
#include "src/Tabs/Experiments/ImPlotDemoTab.hpp"
#include "src/Tabs/Experiments/MeshGenTestTab.hpp"
#include "src/Tabs/Experiments/RendererBasicLightingTab.hpp"
#include "src/Tabs/Experiments/RendererBlendingTab.hpp"
#include "src/Tabs/Experiments/RendererCoordinateSystemsTab.hpp"
#include "src/Tabs/Experiments/RendererFramebuffersTab.hpp"
#include "src/Tabs/Experiments/RendererHelloTriangleTab.hpp"
#include "src/Tabs/Experiments/RendererLightingMapsTab.hpp"
#include "src/Tabs/Experiments/RendererMultipleLightsTab.hpp"
#include "src/Tabs/Experiments/RendererNormalMappingTab.hpp"
#include "src/Tabs/Experiments/RendererSDFTab.hpp"
#include "src/Tabs/Experiments/RendererShadowMappingTab.hpp"
#include "src/Tabs/Experiments/RendererTexturingTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/MeshHittestTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/ModelWarpingTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/PreviewExperimentalDataTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/RendererGeometryShaderTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/TPS2DTab.hpp"
#include "src/OpenSimBindings/Tabs/Experimental/TPS3DTab.hpp"

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

namespace
{
    // an OpenSim log sink that sinks into OSC's main log
    class OpenSimLogSink final : public OpenSim::LogSink {
        void sinkImpl(std::string const& msg) final
        {
            osc::log::info("%s", msg.c_str());
        }
    };

    template<typename TabType>
    void RegisterTab(osc::TabRegistry& registry)
    {
        osc::TabRegistryEntry entry
        {
            TabType::id(),
            [](osc::TabHost* h) { return std::make_unique<TabType>(h); },
        };
        registry.registerTab(entry);
    }

    bool InitializeOpenSim(osc::Config const& config)
    {
        std::filesystem::path geometryDir = config.getResourceDir() / "geometry";

        // these are because OpenSim is inconsistient about handling locales
        //
        // it *writes* OSIM files using the locale, so you can end up with entries like:
        //
        //     <PathPoint_X>0,1323</PathPoint_X>
        //
        // but it *reads* OSIM files with the assumption that numbers will be in the format 'x.y'
        osc::log::info("setting locale to US (so that numbers are always in the format '0.x'");
        char const* locale = "C";
        osc::SetEnv("LANG", locale, 1);
        osc::SetEnv("LC_CTYPE", locale, 1);
        osc::SetEnv("LC_NUMERIC", locale, 1);
        osc::SetEnv("LC_TIME", locale, 1);
        osc::SetEnv("LC_COLLATE", locale, 1);
        osc::SetEnv("LC_MONETARY", locale, 1);
        osc::SetEnv("LC_MESSAGES", locale, 1);
        osc::SetEnv("LC_ALL", locale, 1);
#ifdef LC_CTYPE
        setlocale(LC_CTYPE, locale);
#endif
#ifdef LC_NUMERIC
        setlocale(LC_NUMERIC, locale);
#endif
#ifdef LC_TIME
        setlocale(LC_TIME, locale);
#endif
#ifdef LC_COLLATE
        setlocale(LC_COLLATE, locale);
#endif
#ifdef LC_MONETARY
        setlocale(LC_MONETARY, locale);
#endif
#ifdef LC_MESSAGES
        setlocale(LC_MESSAGES, locale);
#endif
#ifdef LC_ALL
        setlocale(LC_ALL, locale);
#endif
        std::locale::global(std::locale{locale});

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
        osc::log::info("registering OpenSim types");
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();
        RegisterTypes_osimExampleComponents();

        // globally set OpenSim's geometry search path
        //
        // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
        // OpenSim implementation will look in these directories for that file
        osc::log::info("registering OpenSim geometry search path to use osc resources");
        OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometryDir.string());
        osc::log::info("added geometry search path entry: %s", geometryDir.string().c_str());

        // register any user-accessible tabs
        std::shared_ptr<osc::TabRegistry> const registry = osc::App::singleton<osc::TabRegistry>();
        RegisterTab<osc::CustomWidgetsTab>(*registry);
        RegisterTab<osc::HittestTab>(*registry);
        RegisterTab<osc::RendererBasicLightingTab>(*registry);
        RegisterTab<osc::RendererBlendingTab>(*registry);
        RegisterTab<osc::RendererCoordinateSystemsTab>(*registry);
        RegisterTab<osc::RendererFramebuffersTab>(*registry);
        RegisterTab<osc::RendererHelloTriangleTab>(*registry);
        RegisterTab<osc::RendererLightingMapsTab>(*registry);
        RegisterTab<osc::RendererMultipleLightsTab>(*registry);
        RegisterTab<osc::RendererNormalMappingTab>(*registry);
        RegisterTab<osc::RendererTexturingTab>(*registry);
        RegisterTab<osc::RendererSDFTab>(*registry);
        RegisterTab<osc::RendererShadowMappingTab>(*registry);
        RegisterTab<osc::ImGuiDemoTab>(*registry);
        RegisterTab<osc::ImPlotDemoTab>(*registry);
        RegisterTab<osc::ImGuizmoDemoTab>(*registry);
        RegisterTab<osc::MeshGenTestTab>(*registry);
        RegisterTab<osc::MeshHittestTab>(*registry);
        RegisterTab<osc::PreviewExperimentalDataTab>(*registry);
        RegisterTab<osc::RendererGeometryShaderTab>(*registry);
        RegisterTab<osc::TPS2DTab>(*registry);
        RegisterTab<osc::TPS3DTab>(*registry);
        RegisterTab<osc::ModelWarpingTab>(*registry);

        return true;
    }
}


// public API

bool osc::GlobalInitOpenSim(Config const& config)
{
    static bool const s_OpenSimInitialized = InitializeOpenSim(config);
    return s_OpenSimInitialized;
}

osc::OpenSimApp::OpenSimApp() : App{}
{
    GlobalInitOpenSim(getConfig());
}
