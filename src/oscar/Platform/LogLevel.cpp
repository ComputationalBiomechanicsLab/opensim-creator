#include "LogLevel.hpp"

#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <cstddef>

namespace
{
    constexpr auto c_LogLevelStrings = osc::to_array<osc::CStringView>(
    {
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "critical",
        "off",
    });
    static_assert(c_LogLevelStrings.size() == static_cast<size_t>(osc::LogLevel::NUM_LEVELS));
}

osc::CStringView osc::ToCStringView(LogLevel level)
{
    return c_LogLevelStrings.at(static_cast<ptrdiff_t>(level));
}
