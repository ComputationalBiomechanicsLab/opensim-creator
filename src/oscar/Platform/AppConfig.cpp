#include "AppConfig.h"

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/LogLevel.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/CStringView.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

using namespace osc;

namespace
{
    constexpr AntiAliasingLevel c_num_msxaa_samples{4};

    // LEGACY KEYS:
    //
    // - `docs`: used to point to a local directory where the pre-built documentation is
    //   stored. Was changed to `docs_url` to support both local (e.g. file://...) and
    //   internet (e.g. `https://docs.opensimcreator.com`)-based documentation sources
    //
    // - `experimental_feature_flags/multiple_viewports`: used to be a `bool` that indicated
    //   whether the application backend should support multiple viewports, but was
    //   subsequently dropped because multi-viewport support sucked via ImGui on Linux/Mac
    //   (Windows was ok)

    std::filesystem::path get_resources_dir_fallback_path(const AppSettings& settings)
    {
        if (const auto system_config = settings.system_configuration_file_location()) {

            auto resources_dir_path = system_config->parent_path() / "resources";
            if (std::filesystem::exists(resources_dir_path)) {
                return resources_dir_path;
            }
            else {
                log_warn("resources path fallback: tried %s, but it doesn't exist", resources_dir_path.string().c_str());
            }
        }

        auto resources_relative_to_exe = current_executable_directory().parent_path() / "resources";
        if (std::filesystem::exists(resources_relative_to_exe)) {
            return resources_relative_to_exe;
        }
        else {
            log_warn("resources path fallback: using %s as a filler entry, but it doesn't actually exist: the application's configuration file has an incorrect/missing 'resources' key", resources_relative_to_exe.string().c_str());
        }

        return resources_relative_to_exe;
    }

    std::filesystem::path get_resources_dir(const AppSettings& settings)
    {
        // care: the resources directory is _very_, __very__ important
        //
        // if the application can't find resources, then it'll _probably_ fail to
        // boot correctly, which will result in great disappointment, so this code
        // has to try its best

        constexpr CStringView resources_key = "resources";

        const auto resources_dir_setting_value = settings.find_value(resources_key);
        if (not resources_dir_setting_value) {
            return get_resources_dir_fallback_path(settings);
        }

        if (resources_dir_setting_value->type() != AppSettingValueType::String) {
            log_error("application setting for '%s' is not a string: falling back", resources_key.c_str());
            return get_resources_dir_fallback_path(settings);
        }

        // resolve `resources` dir relative to the configuration file in which it was defined
        std::filesystem::path config_file_dir;
        if (const auto p = settings.find_value_filesystem_source(resources_key)) {
            config_file_dir = p->parent_path();
        }
        else {
            config_file_dir = current_executable_directory().parent_path();  // assume the `bin/` dir is one-up from the config
        }
        const std::filesystem::path configured_resources_dir{resources_dir_setting_value->to_string()};

        auto resolved_resource_dir = std::filesystem::weakly_canonical(config_file_dir / configured_resources_dir);
        if (not std::filesystem::exists(resolved_resource_dir)) {
            log_error("'resources', in the application configuration, points to a location that does not exist (%s), so the application may fail to load resources (which is usually a fatal error). Note: the 'resources' path is relative to the configuration file in which you define it (or can be absolute). Attemtping to fallback to a default resources location (which may or may not work).", resolved_resource_dir.string().c_str());
            return get_resources_dir_fallback_path(settings);
        }

        return resolved_resource_dir;
    }

    std::string get_docs_url(const AppSettings& settings)
    {
        if (const auto runtime_url = settings.find_value("docs_url")) {
            return runtime_url->to_string();
        }
        else {
            // TODO: this fallback should be handled in `OpenSimCreator`, not `oscar`
            return "https://docs.opensimcreator.com";
        }
    }

    std::unordered_map<std::string, bool> get_default_panel_states(const AppSettings&)
    {
        return{
            {"Actions", true},
            {"Navigator", true},
            {"Log", true},
            {"Properties", true},
            {"Selection Details", true},
            {"Simulation Details", false},
            {"Coordinates", true},
            {"Performance", false},
            {"Muscle Plot", false},
            {"Output Watches", false},
            {"Output Plots", true},
        };
    }

    std::optional<std::string> get_initial_tab_name(const AppSettings& settings)
    {
        if (const auto v = settings.find_value("initial_tab")) {
            return v->to_string();
        }
        else {
            return std::nullopt;
        }
    }

    LogLevel get_log_level(const AppSettings& settings)
    {
        if (const auto v = settings.find_value("log_level")) {
            return try_parse_as_log_level(v->to_string()).value_or(LogLevel::DEFAULT);
        }
        else {
            return LogLevel::DEFAULT;
        }
    }
}

class osc::AppConfig::Impl final {
public:
    Impl(std::string_view organization_name,
        std::string_view application_name) :
        settings_{organization_name, application_name}
    {}

    AppSettings settings_;
    std::filesystem::path resource_dir_ = get_resources_dir(settings_);
    std::string docs_url_ = get_docs_url(settings_);
    std::unordered_map<std::string, bool> panel_enabled_state_ = get_default_panel_states(settings_);
    std::optional<std::string> maybe_initial_tab_ = get_initial_tab_name(settings_);
    LogLevel log_level_ = get_log_level(settings_);
};


osc::AppConfig::AppConfig(
    std::string_view organization_name,
    std::string_view application_name) :
    impl_{std::make_unique<Impl>(organization_name, application_name)}
{}

osc::AppConfig::AppConfig(AppConfig&&) noexcept = default;
osc::AppConfig& osc::AppConfig::operator=(AppConfig&&) noexcept = default;
osc::AppConfig::~AppConfig() noexcept = default;

std::filesystem::path osc::AppConfig::resource_path(std::string_view resource_name) const
{
    return std::filesystem::weakly_canonical(resource_directory() / resource_name);
}

const std::filesystem::path& osc::AppConfig::resource_directory() const
{
    return impl_->resource_dir_;
}

std::string osc::AppConfig::docs_url() const
{
    return impl_->docs_url_;
}

AntiAliasingLevel osc::AppConfig::anti_aliasing_level() const
{
    return c_num_msxaa_samples;
}

bool osc::AppConfig::is_panel_enabled(const std::string& panel_name) const
{
    return impl_->panel_enabled_state_.try_emplace(panel_name, true).first->second;
}

void osc::AppConfig::set_panel_enabled(const std::string& panel_name, bool v)
{
    impl_->panel_enabled_state_[panel_name] = v;
}

std::optional<std::string> osc::AppConfig::initial_tab_override() const
{
    return impl_->maybe_initial_tab_;
}

LogLevel osc::AppConfig::log_level() const
{
    return impl_->log_level_;
}

std::optional<AppSettingValue> osc::AppConfig::find_value(std::string_view key) const
{
    return impl_->settings_.find_value(key);
}

void osc::AppConfig::set_value(std::string_view key, AppSettingValue value)
{
    impl_->settings_.set_value(key, std::move(value));
}
