#pragma once

#include "oscar/Graphics/AntiAliasingLevel.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace osc
{
    class AppConfig final {
    public:
        // try to load the config from disk (default location)
        static std::unique_ptr<AppConfig> load();

        class Impl;
    public:
        explicit AppConfig(std::unique_ptr<Impl>);  // you should use AppConfig::load
        AppConfig(AppConfig const&) = delete;
        AppConfig(AppConfig&&) noexcept;
        AppConfig& operator=(AppConfig const&) = delete;
        AppConfig& operator=(AppConfig&&) noexcept;
        ~AppConfig() noexcept;

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


    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
