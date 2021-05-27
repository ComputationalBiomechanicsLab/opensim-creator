#pragma once

#include "src/assertions.hpp"

#include <utility>
#include <filesystem>
#include <memory>

namespace osc {

    template<typename T>
    struct Config_option final {
        T value;
        bool is_dirty = false;

        explicit Config_option(T value_) : value{std::move(value_)} {
        }

        operator T&() noexcept {
            return value;
        }

        operator T const&() const noexcept {
            return value;
        }
    };

    // runtime configuration for the application
    struct Config {
        Config_option<std::filesystem::path> resource_dir;
        Config_option<bool> use_multi_viewport;

        // default-init with compiled-in values
        Config();
    };

    // global for the application
    //
    // statically initialized with compiled-in defaults for each config option
    extern std::unique_ptr<Config> g_ApplicationConfig;

    // loads configuration via environment, files, etc.
    void init_load_config();

    // get runtime config
    [[nodiscard]] static inline Config const& config() noexcept {
        OSC_ASSERT(g_ApplicationConfig != nullptr);
        return *g_ApplicationConfig;
    }

    // get (mutable access to) runtime config
    [[nodiscard]] static inline Config& upd_config() noexcept {
        OSC_ASSERT(g_ApplicationConfig != nullptr);
        return *g_ApplicationConfig;
    }
}
