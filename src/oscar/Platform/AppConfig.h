#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Platform/LogLevel.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace osc
{
    class AppConfig final {
    public:
        class Impl;

        AppConfig(
            std::string_view organization_name,
            std::string_view application_name
        );
        AppConfig(const AppConfig&) = delete;
        AppConfig(AppConfig&&) noexcept;
        AppConfig& operator=(const AppConfig&) = delete;
        AppConfig& operator=(AppConfig&&) noexcept;
        ~AppConfig() noexcept;

        // returns canonicalized path to the given resource key (e.g. `a/b/c`)
        std::filesystem::path resource_path(std::string_view) const;

        // returns the full path to the runtime `resources/` dir
        const std::filesystem::path& resource_directory() const;

        // returns a URL to the application's documentation (if available)
        std::string docs_url() const;

        // returns the number of anti-aliasing samples that 3D viewports should use
        AntiAliasingLevel anti_aliasing_level() const;

        // returns `true` if a given UI panel with name `panel_name` is enabled or not
        bool is_panel_enabled(const std::string& panel_name) const;
        void set_panel_enabled(const std::string& panel_name, bool);

        // returns the name of a `Tab` that should be opened upon booting (overriding default behavior)
        std::optional<std::string> initial_tab_override() const;

        // returns the user-requested log level that the application should be initialized with
        LogLevel log_level() const;

        // returns arbitrary runtime configuration values that should be persisted between
        // application sessions
        std::optional<AppSettingValue> find_value(std::string_view key) const;
        void set_value(std::string_view key, AppSettingValue value);

    private:
        std::unique_ptr<Impl> impl_;
    };
}
