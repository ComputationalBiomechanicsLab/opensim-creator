#include "opynsim.h"

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>
#include <jam-plugin/Smith2018ArticularContactForce.h>
#include <jam-plugin/Smith2018ContactMesh.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/conversion.h>

#if defined(WIN32)
#include <Windows.h>  // `GetEnvironmentVariableA` / `SetEnvironmentVariableA`
#else
#include <cstdlib>  // `setenv`
#endif

#include <clocale>
#include <cstring>
#include <iostream>
#include <locale>
#include <sstream>
#include <utility>

using namespace opyn;

// An `osc::Converter` that maps `spdlog::level`s (from OpenSim) to
// `oscar`'s `LogLevel`.
template<>
struct osc::Converter<spdlog::level::level_enum, osc::LogLevel> {
    osc::LogLevel operator()(spdlog::level::level_enum e) const
    {
        switch (e) {
        case spdlog::level::level_enum::trace:    return osc::LogLevel::trace;
        case spdlog::level::level_enum::debug:    return osc::LogLevel::debug;
        case spdlog::level::level_enum::info:     return osc::LogLevel::info;
        case spdlog::level::level_enum::warn:     return osc::LogLevel::warn;
        case spdlog::level::level_enum::err:      return osc::LogLevel::err;
        case spdlog::level::level_enum::critical: return osc::LogLevel::critical;
        case spdlog::level::level_enum::off:      return osc::LogLevel::off;
        default:                                  return osc::LogLevel::DEFAULT;
        }
    }
};

// An `osc::Converter` that maps `spdlog::string_view_t`s (from OpenSim) to
// `std::string`s.
template<>
struct osc::Converter<spdlog::string_view_t, std::string> {
    std::string operator()(spdlog::string_view_t s) const
    {
        return {s.begin(), s.end()};
    }
};

namespace
{
    // An OpenSim log sink that sinks into the `oscar` application log.
    class OpenSimLogSink final : public OpenSim::LogSink {
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            osc::log_message(
                osc::to<osc::LogLevel>(msg.level),
                osc::to<std::string>(msg.payload)
            );
        }
        void flush_() override {}
    };

    // Globally mutates OpenSim's logging configuration to use the
    // `oscar` log instead of its default.
    void setup_opensim_to_use_oscar_log()
    {
        // Disable OpenSim's `opensim.log` default.
        //
        // By default, OpenSim creates an `opensim.log` file in the process's working
        // directory. This should be disabled because it screws with running multiple
        // instances of the UI on filesystems that lock files (e.g. NTFS on Windows)
        // and because it's incredibly obnoxious to have `opensim.log` appear in
        // working directories.
        OpenSim::Logger::removeFileSink();

        // Add an OpenSim log sink that sinks to `oscar`'s global log.
        //
        // This centralizes logging to the `oscar` logging system, so that callers
        // can control logging from one place.
        OpenSim::Logger::addSink(std::make_shared<OpenSimLogSink>());
    }

    // Helper function that sets one environment variable unsafely.
    int setenv_wrapper(const char* name, const char* value, bool overwrite)
    {
        // Input validation
        if (!name || *name == '\0' || std::strchr(name, '=') != nullptr || !value) {
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
        return setenv(name, value, overwrite ? 1 : 0);  // NOLINT(concurrency-mt-unsafe)
#endif
    }

    // Helper function that wraps  `std::setlocale` so that any linter complaints
    // about multithreaded unsafety  are all deduped to this one source location.
    //
    // It's unsafe because `setlocale` globally mutates environment state.
    void setlocale_wrapper(int category, const char* locale)
    {
        if (std::setlocale(category, locale) == nullptr) { // NOLINT(concurrency-mt-unsafe)
            std::stringstream content;
            content << "error setting locale category " << category << " to " << locale;
            osc::log_warn(std::move(content).str());
        }
    }

    // Globally sets the process's locale so that it is consistent about how
    // it loads numeric data from files.
    //
    // This is necessary because OpenSim is inconsistent about how it handles
    // locales. Sometimes it writes numbers according to the user's locale (e.g.
    // comma separator for decimal place) but then reads it according to the
    // general US locale (e.g. the separator is always a period), causing problems.
    void set_global_locale_to_match_OpenSim()
    {
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

    // Globally adds all known components to OpenSim's global
    // component registry in `OpenSim::Object`, so that OpenSim
    // is capable of loading all components via XML files.
    void register_all_components_with_opensim_object_registry()
    {
        RegisterTypes_osimCommon();
        RegisterTypes_osimSimulation();
        RegisterTypes_osimActuators();
        RegisterTypes_osimAnalyses();
        RegisterTypes_osimTools();
        RegisterTypes_osimExampleComponents();
        OpenSim::Object::registerType(OpenSim::Smith2018ArticularContactForce());
        OpenSim::Object::registerType(OpenSim::Smith2018ContactMesh());
    }

    // Globally ensures that OpenSim's log is initialized exactly once to
    // use the `oscar` log (can be called multiple times).
    void globally_ensure_log_is_default_initialized()
    {
        [[maybe_unused]] static bool s_log_initialized = []()
        {
            osc::global_default_logger()->set_level(osc::LogLevel::err);
            setup_opensim_to_use_oscar_log();
            return true;
        }();
    }
}

void opyn::set_log_level(osc::LogLevel log_level)
{
    globally_ensure_log_is_default_initialized();
    osc::global_default_logger()->set_level(log_level);
}

void opyn::add_geometry_directory(const std::filesystem::path& directory)
{
    OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(directory.string());
    osc::log_info("added geometry search path entry: %s", directory.string().c_str());
}

bool opyn::init()
{
    // Ensure the log is *at least* default-initialized. Callers might be able to
    // do this before `init` is called.
    globally_ensure_log_is_default_initialized();

    // This part should only ever be called once per process.
    static bool s_osc_initialized = []
    {
        osc::log_info("initializing OPynSim (opyn::init)");

        // Make the current process globally use the same locale that OpenSim uses
        //
        // This is necessary because OpenSim assumes a certain locale (see function
        // impl. for more details)
        set_global_locale_to_match_OpenSim();

        // Register all OpenSim components with the `OpenSim::Object` registry.
        register_all_components_with_opensim_object_registry();

        return true;
    }();

    return s_osc_initialized;
}

ModelSpecification opyn::import_osim_file(const std::filesystem::path& osim_file_path)
{
    return ModelSpecification::from_osim_file(osim_file_path);
}
