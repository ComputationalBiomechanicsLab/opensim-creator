#include "opynsim.h"

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <libopynsim/data_frame.h>
#include <libopynsim/model_specification.h>

#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/FileAdapter.h>
#include <OpenSim/Common/DataTable.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/Common/TimeSeriesTable.h>
#include <OpenSim/ExampleComponents/RegisterTypes_osimExampleComponents.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>
#include <jam-plugin/Smith2018ArticularContactForce.h>
#include <jam-plugin/Smith2018ContactMesh.h>
#include <liboscar/formats/csv.h>
#include <liboscar/formats/image.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/string_helpers.h>

#if defined(WIN32)
#include <Windows.h>  // `GetEnvironmentVariableA` / `SetEnvironmentVariableA`
#else
#include <cstdlib>  // `setenv`
#endif
#include <clocale>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

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

// An `osc::Converter` that reads a `SimTK::VectorView` into a `std::vector<double>`
template<>
struct osc::Converter<SimTK::VectorView, std::vector<double>> {
    std::vector<double> operator()(const SimTK::VectorView& view) const
    {
        std::vector<double> rv;
        rv.reserve(view.size());
        for (int i = 0; i < view.size(); ++i) {
            rv.push_back(view[i]);
        }
        return rv;
    }
};

namespace
{
    // An OpenSim log sink that sinks into the `oscar` application log.
    class OpenSimLogSink final : public OpenSim::LogSink {
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            osc::log_message(osc::to<osc::LogLevel>(msg.level), osc::to<std::string>(msg.payload));
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
            osc::log_warn("error setting locale category {} to {}", category, locale);
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
        osc::log_info("setting locale to US (so that numbers are always in the format '0.x')");
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

    DataFrame read_opensim_datatable_into_data_frame(
        const OpenSim::DataTable_<double, double>& table)
    {
        // Transform source file into `opyn::DataFrame` inputs.
        std::vector<std::string> column_names;
        column_names.reserve(table.getNumColumns());
        std::vector<std::vector<double>> column_data;
        column_data.reserve(table.getNumColumns());

        // Read `time` column (always present in `OpenSim::TimeSeriesTable`).
        column_names.emplace_back("time");
        column_data.push_back(table.getIndependentColumn());

        // Read data columns (dependent columns)
        for (size_t i = 0; i < table.getNumColumns(); ++i) {
            column_names.push_back(table.getColumnLabel(i));
            const SimTK::VectorView view = table.getDependentColumnAtIndex(i);
            const auto data = osc::to<std::vector<double>>(view);
            column_data.push_back(data);
        }

        // Read metadata (attributes)
        std::unordered_map<std::string, std::string> attrs;
        {
            const auto& metadata = table.getTableMetaData();
            auto keys = metadata.getKeys();
            attrs.reserve(keys.size());
            for (auto& key : keys) {
                auto value = metadata.getValueAsString(key);

                attrs.insert_or_assign(std::move(key), std::move(value));
            }
        }

        // Clean out blank "header" metadata (OpenSim adds it but a file isn't strictly
        // required to have anything in it).
        if (const auto it = attrs.find("header"); it != attrs.end()) {
            if (it->second.empty()) {
                attrs.erase(it);
            }
        }

        // If the file contains an `inDegrees` attribute, normalize it such that "y"
        // and "yes" (case-insensitive) always become "yes" and anything else always
        // becomes "no"
        if (const auto it = attrs.find("inDegrees"); it != attrs.end()) {

            if (   osc::is_equal_case_insensitive(it->second, "y")
                or osc::is_equal_case_insensitive(it->second, "yes")) {

                it->second = "yes";
            } else {
                it->second = "no";
            }
        }

        // If the file is exactly version 0 and contains a special angles substring
        // in the header then that should be coerced into an `inDegrees` key. This
        // is a backwards compatibility shim because `OpenSim::Storage` works that
        // way.
        if (not attrs.contains("inDegrees")
            and attrs.contains("version")
            and attrs["version"] == "0"
            and attrs.contains("header")) {

            if (attrs["header"].contains("Angles are in degrees.")) {
                attrs["inDegrees"] = "yes";
            } else if (attrs["header"].contains("Angles are in radians.")) {
                attrs["inDegrees"] = "no";
            }
        }

        // Remove these keys: they're used to parse the file into memory, but once
        // it's parsed, whatever's re-emitted will have whatever the current version
        // is.
        attrs.erase("version");
        attrs.erase("OpenSimVersion");

        return DataFrame{
            std::move(column_names),
            std::move(column_data),
            std::move(attrs),
        };
    }

    DataFrame read_dataframe_via_opensim_file_adaptor(
        const std::filesystem::path& source,
        std::optional<std::string_view> table_name = std::nullopt)
    {
        const auto tables = OpenSim::FileAdapter::createAdapterFromExtension(source.string())->read(source.string());
        if (tables.size() > 1 and not table_name) {
            std::stringstream ss;
            ss << source << ": contains more than one table (";
            std::string_view delim;
            for (const auto& entry : tables) {
                ss << delim << entry.first;
                delim = ", ";
            }
            ss << ") but no table name was specified.";
            throw std::runtime_error{std::move(ss).str()};
        }

        const OpenSim::AbstractDataTable* table{};
        if (table_name) {
            const auto it = tables.find(std::string{*table_name});
            if (it == tables.end()) {
                std::stringstream ss;
                ss << source << "Could not find table '" << *table_name << "' in the data source";
                throw std::runtime_error{std::move(ss).str()};
            }
            table = it->second.get();
        } else {
            table = tables.begin()->second.get();
        }

        // OPynSim always tries to flatten tables with suffixes, so that
        // users always see one consistent `DataFrame` type. It's simpler
        // than OpenSim's typed data table API but easier to work with in
        // the wider Python ecosystem.
        OSC_ASSERT(table != nullptr && "Earlier code should have set this");
        if (const auto* doubles_table = dynamic_cast<const OpenSim::DataTable_<double, double>*>(table)) {
            return read_opensim_datatable_into_data_frame(*doubles_table);
        }
        if (const auto* vec3_table = dynamic_cast<const OpenSim::DataTable_<double, SimTK::Vec3>*>(table)) {
            return read_opensim_datatable_into_data_frame(vec3_table->flatten({"_x", "_y", "_z"}));
        }
        if (const auto* quaternion_table = dynamic_cast<const OpenSim::DataTable_<double, SimTK::Quaternion>*>(table)) {
            return read_opensim_datatable_into_data_frame(quaternion_table->flatten({"_w", "_x", "_y", "_z"}));
        }

        std::stringstream ss;
        ss << source << ": Specified data table has an unsupported data type (" << typeid(*table).name() << ").";
        throw std::runtime_error{std::move(ss).str()};
    }

    osc::Texture2D read_texture_via_oscar(const std::filesystem::path& source)
    {
        std::ifstream ifs{source, std::ios::in | std::ios::binary};
        if (not ifs) {
            std::stringstream ss;
            ss << source << ": Error opening input file";
            throw std::runtime_error{std::move(ss).str()};
        }

        return osc::Image::read_into_texture(ifs, source.filename().string(), osc::ColorSpace::sRGB);
    }
}

osc::LogLevel opyn::get_log_level()
{
    globally_ensure_log_is_default_initialized();
    return osc::global_default_logger()->level();
}

void opyn::set_log_level(osc::LogLevel log_level)
{
    globally_ensure_log_is_default_initialized();
    osc::global_default_logger()->set_level(log_level);
}

std::vector<std::filesystem::path> opyn::get_search_paths()
{
    return OpenSim::ModelVisualizer::getSearchPaths();
}

void opyn::prepend_search_path(std::filesystem::path search_path)
{
    OpenSim::ModelVisualizer::prependSearchPath(std::move(search_path));
}

void opyn::append_search_path(std::filesystem::path search_path)
{
    OpenSim::ModelVisualizer::appendSearchPath(std::move(search_path));
}

bool opyn::remove_search_path(const std::filesystem::path& search_path)
{
    return OpenSim::ModelVisualizer::removeSearchPath(search_path);
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

ModelSpecification opyn::read_osim(const std::filesystem::path& source)
{
    return ModelSpecification::from_osim_file(source);
}

DataFrame opyn::read_sto(const std::filesystem::path& source)
{
    return read_dataframe_via_opensim_file_adaptor(source);
}

DataFrame opyn::read_mot(const std::filesystem::path& source)
{
    return read_dataframe_via_opensim_file_adaptor(source);
}

DataFrame opyn::read_trc(const std::filesystem::path& source)
{
    return read_dataframe_via_opensim_file_adaptor(source);
}

DataFrame opyn::read_csv(const std::filesystem::path& source)
{
    return read_dataframe_via_opensim_file_adaptor(source);
}

osc::Mesh opyn::read_vtp(const std::filesystem::path& source)
{
    return LoadMeshViaSimbody(source);
}

osc::Mesh opyn::read_obj(const std::filesystem::path& source)
{
    return LoadMeshViaSimbody(source);
}

osc::Mesh opyn::read_stl(const std::filesystem::path& source)
{
    return LoadMeshViaSimbody(source);
}

osc::Texture2D opyn::read_png(const std::filesystem::path& source)
{
    return read_texture_via_oscar(source);
}

osc::Texture2D opyn::read_jpeg(const std::filesystem::path& source)
{
    return read_texture_via_oscar(source);
}

osc::Texture2D opyn::read_jpg(const std::filesystem::path& source)
{
    return read_jpeg(source);
}
