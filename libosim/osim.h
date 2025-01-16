#pragma once

#include <string_view>

namespace osim
{
    enum class LogLevel {
        info,
        warn,
        NUM_OPTIONS,
    };

    // Runtime configuration that can be given to `osim::init` to change/monitor
    // its behavior.
    class InitConfiguration {
    public:
        virtual ~InitConfiguration() noexcept = default;

        // Called when `osim::init` wants to emit an informational log message
        void log_info(std::string_view payload) { impl_log_message(payload, LogLevel::info); }

        // Called when `osim::init` wants to emit a warning log message
        void log_warn(std::string_view payload) { impl_log_message(payload, LogLevel::warn); }
    private:

        // Implementors can override this to provide custom message logging behavior.
        //
        // By default, it writes messages to `std::cerr`.
        virtual void impl_log_message(std::string_view, LogLevel);
    };

    // Globally initializes the osim (OpenSim + extensions) API with a default `InitConfiguration`.
    //
    // This should be called by the application exactly once before using any `osim::`,
    // `SimTK::`, or `OpenSim::`-prefixed API.
    void init();

    // Globally initializes the osim (OpenSim + extensions) API with the given `InitConfiguration`.
    //
    // This should be called by the application exactly once before using any `osim::`,
    // `SimTK::`, or `OpenSim::`-prefixed API.
    void init(InitConfiguration&);
}
