#pragma once

#include <oscar/Platform/AppSettingsStatus.hpp>
#include <oscar/Platform/AppSettingValue.hpp>

#include <optional>
#include <string_view>

namespace osc
{
    // persistent, platform-independent, application settings
    class AppSettings final {
    public:
        AppSettings(
            std::string_view organization_,
            std::string_view applicationName_
        );
        AppSettings(AppSettings const&);
        AppSettings(AppSettings&&) noexcept;
        AppSettings& operator=(AppSettings const&);
        AppSettings& operator=(AppSettings&&) noexcept;
        ~AppSettings() noexcept;  // try to sync on destruction

        std::optional<AppSettingValue> getValue(
            std::string_view key
        ) const;

        void setValue(
            std::string_view key,
            AppSettingValue
        );

        void sync();

        AppSettingsStatus status() const;
    };
}
