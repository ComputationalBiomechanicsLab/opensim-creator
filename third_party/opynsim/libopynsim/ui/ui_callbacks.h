#pragma once

#include <functional>

namespace opyn
{
    /// Represents a collection of callback functions that C++ UI
    /// implementations call during the lifecycle of the UI. Useful
    /// for intermittently checking external systems.
    struct UiCallbacks final {

        /// Called each time the UI "ticks" (i.e. once per frame, before
        /// drawing anything).
        std::function<void()> on_tick_begin = []{};
    };
}
