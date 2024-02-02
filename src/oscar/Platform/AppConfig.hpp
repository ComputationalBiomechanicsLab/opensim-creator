#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Platform/LogLevel.hpp>

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
    public:
        AppConfig(
            std::string_view organizationName_,
            std::string_view applicationName_
        );
        AppConfig(AppConfig const&) = delete;
        AppConfig(AppConfig&&) noexcept;
        AppConfig& operator=(AppConfig const&) = delete;
        AppConfig& operator=(AppConfig&&) noexcept;
        ~AppConfig() noexcept;

        // returns canonicalized path to the given resource key (e.g. a/b/c)
        std::filesystem::path getResourcePath(std::string_view k) const;

        // get the full path to the runtime `resources/` dir
        std::filesystem::path const& getResourceDir() const;

        // get the full path to the runtime `html/` dir for the documentation
        std::filesystem::path const& getHTMLDocsDir() const;

        // returns true if the implementation should allow multiple viewports
        bool isMultiViewportEnabled() const;

        // get number of MSXAA antiAliasingLevel 3D viewports should use
        AntiAliasingLevel getNumMSXAASamples() const;

        // get if a given UI panel is enabled or not
        bool getIsPanelEnabled(std::string const& panelName) const;
        void setIsPanelEnabled(std::string const& panelName, bool);

        // get the name of a tab that should be opened upon booting (overriding default behavior)
        std::optional<std::string> getInitialTabOverride() const;

        // get the user-requested log level that the application should be initialized with
        LogLevel getRequestedLogLevel() const;

        // get/set arbitrary runtime configuration values that should be persisted between
        // application sessions
        std::optional<AppSettingValue> getValue(
            std::string_view key
        ) const;
        void setValue(
            std::string_view key,
            AppSettingValue value
        );

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
