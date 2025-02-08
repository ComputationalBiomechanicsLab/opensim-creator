#include "osim.h"

#include <libosim/ThirdPartyPlugins/RegisterTypes_osimPlugin.h>

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
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

using namespace osim;

namespace
{
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
    void setlocale_wrapper(int category, const char* locale, InitConfiguration& config)
    {
        // disable lint because this function is only called once at application
        // init time
        if (std::setlocale(category, locale) == nullptr) { // NOLINT(concurrency-mt-unsafe)
            std::stringstream content;
            content << "error setting locale category " << category << " to " << locale;
            config.log_warn(std::move(content).str());
        }
    }

    void set_global_locale_to_match_OpenSim(InitConfiguration& config)
    {
        // these are because OpenSim is inconsistient about handling locales
        //
        // it *writes* OSIM files using the locale, so you can end up with entries like:
        //
        //     <PathPoint_X>0,1323</PathPoint_X>
        //
        // but it *reads* OSIM files with the assumption that numbers will be in the format 'x.y'

        config.log_info("setting locale to US (so that numbers are always in the format '0.x'");
        const char* locale = "C";
        for (const char* envvar : {"LANG", "LC_CTYPE", "LC_NUMERIC", "LC_TIME", "LC_COLLATE", "LC_MONETARY", "LC_MESSAGES", "LC_ALL"}) {
            setenv_wrapper(envvar, locale, true);
        }
#ifdef LC_CTYPE
        setlocale_wrapper(LC_CTYPE, locale, config);
#endif
#ifdef LC_NUMERIC
        setlocale_wrapper(LC_NUMERIC, locale, config);
#endif
#ifdef LC_TIME
        setlocale_wrapper(LC_TIME, locale, config);
#endif
#ifdef LC_COLLATE
        setlocale_wrapper(LC_COLLATE, locale, config);
#endif
#ifdef LC_MONETARY
        setlocale_wrapper(LC_MONETARY, locale, config);
#endif
#ifdef LC_MESSAGES
        setlocale_wrapper(LC_MESSAGES, locale, config);
#endif
#ifdef LC_ALL
        setlocale_wrapper(LC_ALL, locale, config);
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
        RegisterTypes_osimPlugin();  // from `ThirdPartyPlugins/`
    }

    std::string_view label_for(LogLevel level)
    {
        switch (level) {
        case LogLevel::info: return "INFO";
        case LogLevel::warn: return "WARN";
        default:             return "INFO";
        }
    }
}

void osim::init()
{
    InitConfiguration config;
    init(config);
}

void osim::InitConfiguration::impl_log_message(std::string_view payload, LogLevel level)
{
    std::cerr << label_for(level) << ": " << payload << std::endl;
}

void osim::init(InitConfiguration& config)
{
    // make the current process globally use the same locale that OpenSim uses
    //
    // this is necessary because OpenSim assumes a certain locale (see function
    // impl. for more details)
    set_global_locale_to_match_OpenSim(config);

    // register all OpenSim components with the `OpenSim::Object` registry
    RegisterTypes_all();
}
