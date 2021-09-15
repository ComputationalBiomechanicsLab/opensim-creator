#pragma once

#include <filesystem>
#include <memory>

namespace osc {
    struct Config {
        static std::unique_ptr<Config> load();

        // full path to the runtime `resources/` dir
        std::filesystem::path resourceDir;

        // full path to the runtime `html/` dir for the documentation
        std::filesystem::path htmlDocsDir;

        // true if the implementation should allow multiple viewports
        bool useMultiViewport;

        // number of MSXAA samples 3D viewports should use
        static constexpr int numMSXAASamples = 8;
    };
}
