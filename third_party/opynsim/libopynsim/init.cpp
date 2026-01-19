#include "init.h"

#include <jam-plugin/Smith2018ArticularContactForce.h>
#include <jam-plugin/Smith2018ContactMesh.h>
#include <liboscar/platform/log.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>

#if defined(WIN32)
#include <Windows.h>  // `GetEnvironmentVariableA` / `SetEnvironmentVariableA`
#else
#include <stdlib.h>  // `setenv`
#endif

#include <clocale>
#include <cstring>
#include <iostream>
#include <locale>
#include <sstream>
#include <utility>

using namespace opyn;

namespace
{
    // An OpenSim log sink that sinks into the `oscar` application log.
    class OpenSimLogSink final : public OpenSim::LogSink {
        void sinkImpl(const std::string& msg) final
        {
            osc::log_info("%s", msg.c_str());
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
        OpenSim::Logger::removeFileSink();

        // add OSC in-memory logger
        //
        // this logger collects the logs into a global mutex-protected in-memory structure
        // that the UI can can trivially render (w/o reading files etc.)
        OpenSim::Logger::addSink(std::make_shared<OpenSimLogSink>());
    }

    int setenv_wrapper(const char* name, const char* value, int overwrite)
    {
        // Input validation
        if (!name || *name == '\0' || std::strchr(name, '=') != NULL || !value) {
            return -1;
        }

#if defined(WIN32)
        if (!overwrite) {
            if (GetEnvironmentVariableA(name, NULL, 0) > 0) {
                return 0; /* asked not to overwrite existing value. */
            }
        }
        if (!SetEnvironmentVariableA(name, *value ? value : NULL)) {
            return -1;
        }
        return 0;
#else
        return setenv(name, value, overwrite);
#endif
    }

    // minor alias for `std::setlocale` so that any linter complaints about MT unsafety
    // are all deduped to this one source location
    //
    // it's UNSAFE because `setlocale` is a global mutation
    void setlocale_wrapper(int category, const char* locale)
    {
        // disable lint because this function is only called once at application
        // init time
        if (std::setlocale(category, locale) == nullptr) { // NOLINT(concurrency-mt-unsafe)
            std::stringstream content;
            content << "error setting locale category " << category << " to " << locale;
            osc::log_warn(std::move(content).str());
        }
    }

    void set_global_locale_to_match_OpenSim()
    {
        // these are because OpenSim is inconsistient about handling locales
        //
        // it *writes* .osim files using the locale, so you can end up with entries like:
        //
        //     <PathPoint_X>0,1323</PathPoint_X>
        //
        // but it *reads* .osim files with the assumption that numbers will be in the format 'x.y'

        osc::log_info("setting locale to US (so that numbers are always in the format '0.x'");
        const char* locale = "C";
        for (const char* envvar : {"LANG", "LC_CTYPE", "LC_NUMERIC", "LC_TIME", "LC_COLLATE", "LC_MONETARY", "LC_MESSAGES", "LC_ALL"}) {
            setenv_wrapper(envvar, locale, true);
        }
#ifdef LC_CTYPE
        setlocale_wrapper(LC_CTYPE, locale);
#endif
#ifdef LC_NUMERIC
        setlocale_wrapper(LC_NUMERIC, locale);
#endif
#ifdef LC_TIME
        setlocale_wrapper(LC_TIME, locale);
#endif
#ifdef LC_COLLATE
        setlocale_wrapper(LC_COLLATE, locale);
#endif
#ifdef LC_MONETARY
        setlocale_wrapper(LC_MONETARY, locale);
#endif
#ifdef LC_MESSAGES
        setlocale_wrapper(LC_MESSAGES, locale);
#endif
#ifdef LC_ALL
        setlocale_wrapper(LC_ALL, locale);
#endif
        std::locale::global(std::locale{locale});
    }

    void RegisterTypes_all()
    {
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();
        RegisterTypes_osimExampleComponents();
        OpenSim::Object::registerType( OpenSim::Smith2018ArticularContactForce() );
        OpenSim::Object::registerType( OpenSim::Smith2018ContactMesh() );
    }

    void globally_ensure_log_is_default_initialized()
    {
        [[maybe_unused]] static bool s_log_initialized = []()
        {
            SetupOpenSimLogToUseOSCsLog();
            return true;
        }();
    }
}

bool opyn::init()
{
    // Ensure the log is *at least* default-initialized.
    globally_ensure_log_is_default_initialized();

    osc::log_info("initializing OPynSim (opyn::init)");

    // Make the current process globally use the same locale that OpenSim uses
    //
    // This is necessary because OpenSim assumes a certain locale (see function
    // impl. for more details)
    set_global_locale_to_match_OpenSim();

    // Register all OpenSim components with the `OpenSim::Object` registry.
    RegisterTypes_all();

    return true;
}
