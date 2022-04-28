#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace osc
{
    class Config final {
    public:
        // try to load the config from disk (default location)
        static std::unique_ptr<Config> load();

        class Impl;
    public:
        explicit Config(Impl*);  // you should use Config::load
        Config(Config const&) = delete;
        Config(Config&&) noexcept;
        Config& operator=(Config const&) = delete;
        Config& operator=(Config&&) noexcept;
        ~Config() noexcept;

        // get the full path to the runtime `resources/` dir
        std::filesystem::path const& getResourceDir() const;

        // get the full path to the runtime `html/` dir for the documentation
        std::filesystem::path const& getHTMLDocsDir() const;

        // returns true if the implementation should allow multiple viewports
        bool isMultiViewportEnabled() const;

        // get number of MSXAA samples 3D viewports should use
        int getNumMSXAASamples() const;

        // get if a given UI panel is enabled or not
        bool getIsPanelEnabled(std::string const& panelName) const;
        void setIsPanelEnabled(std::string const& panelName, bool v);

    private:
        Impl* m_Impl;
    };
}
