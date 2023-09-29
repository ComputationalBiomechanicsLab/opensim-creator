#pragma once

#include <oscar/Platform/AppSettingValue.hpp>

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

        std::optional<AppSettingValue> getValue(
            std::string_view key
        ) const;

        void setValue(
            std::string_view key,
            AppSettingValue
        );

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
