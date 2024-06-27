#pragma once

#include <oscar/Platform/AppSettingScope.h>
#include <oscar/Platform/AppSettingValue.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace osc
{
    // persistent, platform-independent, singleton-ed application settings
    class AppSettings final {
    public:
        AppSettings(
            std::string_view organization_name,
            std::string_view application_name,
            std::string_view application_config_file_name = "osc.toml"
        );
        AppSettings(const AppSettings&);
        AppSettings(AppSettings&&) noexcept;
        AppSettings& operator=(const AppSettings&);
        AppSettings& operator=(AppSettings&&) noexcept;
        ~AppSettings() noexcept;

        // if available, returns the filesystem path of the system configuration file
        //
        // the system configuration file isn't necessarily available (e.g. the user
        // may have deleted it)
        std::optional<std::filesystem::path> system_configuration_file_location() const;

        std::optional<AppSettingValue> find_value(std::string_view key) const;
        void set_value(std::string_view key, AppSettingValue, AppSettingScope = AppSettingScope::User);
        void set_value_if_not_found(std::string_view key, AppSettingValue, AppSettingScope = AppSettingScope::User);

        // if available, returns the filesystem path of the configuration file that
        // provided the given setting value
        //
        // this can be useful if (e.g.) the value is specifying something that is
        // relative to the configuration file's location on disk
        //
        // not available if:
        //
        // - `key` isn't set
        // - `key` is set, but `AppSettings` was unable to find/create a suitable
        //   user configuration file (e.g. user filesystem permissions are wrong)
        std::optional<std::filesystem::path> find_value_filesystem_source(std::string_view key) const;

        // synchronize the current in-memory state of this settings object to disk
        //
        // note #1: this is automatically called by the destructor
        //
        // note #2: only user-level and values that were set with `setValue` will
        //          be synchronized to disk - system values are not synchronized.
        void sync();

        class Impl;
    private:
        std::shared_ptr<Impl> impl_;
    };

    // returns a filesystem path to the application's `resources/` directory. Uses heuristics
    // to figure out where it is if the provided `AppSettings` doesn't contain the necessary
    // information
    std::filesystem::path get_resource_dir_from_settings(const AppSettings&);
}
