#pragma once

#include <chrono>

namespace osc
{
    // A `std::chrono`-compatible clock that is steady (i.e. always monotonically
    // increases) and has a high-resolution enough for performance measurements.
    using PerfClock = std::chrono::steady_clock;
}
