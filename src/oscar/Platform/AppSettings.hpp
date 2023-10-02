#pragma once

#include <oscar/Platform/AppSettingValue.hpp>

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
            std::string_view organizationName_,
            std::string_view applicationName_
        );
        AppSettings(AppSettings const&);
        AppSettings(AppSettings&&) noexcept;
        AppSettings& operator=(AppSettings const&);
        AppSettings& operator=(AppSettings&&) noexcept;
        ~AppSettings() noexcept;  // try to sync on destruction

        // if available, returns the filesystem path of the system configuration file
        //
        // the system configuration file isn't necessarily available (e.g. the user
        // may have deleted it)
        std::optional<std::filesystem::path> getSystemConfigurationFileLocation() const;

        std::optional<AppSettingValue> getValue(
            std::string_view key
        ) const;

        void setValue(
            std::string_view key,
            AppSettingValue
        );

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
        //   user configuration file (e.g. user filesystem permissions are borked)
        std::optional<std::filesystem::path> getValueFilesystemSource(
            std::string_view key
        ) const;

        // TODO: `void sync()`
        //
        // - extract all user-defined app settings
        // - sort alphabetically
        // - for each entry:
        //   - A if entry key contains `/`
        //     - for each entry with same prefix up to `/`
        //         if entry key contains `/` ... (recursive)
        //   - else (recursion bottom-out)
        //     - emit value

        class Impl;
    private:
        std::shared_ptr<Impl> m_Impl;
    };
}
