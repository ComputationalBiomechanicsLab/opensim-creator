#pragma once

#include <filesystem>
#include <memory>

namespace osc {
    struct Config {
        static std::unique_ptr<Config> load();

        // full path to the runtime `resources/` dir
        std::filesystem::path resource_dir;

        // true if the implementation should allow multiple viewports
        bool use_multi_viewport;

        // number of MSXAA samples 3D viewports should use
        static constexpr int msxaa_samples = 8;
    };
}
