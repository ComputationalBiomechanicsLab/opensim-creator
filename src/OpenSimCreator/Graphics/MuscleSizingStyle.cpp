#include "MuscleSizingStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    auto constexpr c_Styles = osc::to_array<osc::MuscleSizingStyle>(
    {
        osc::MuscleSizingStyle::OpenSim,
        osc::MuscleSizingStyle::PcsaDerived,
    });
    static_assert(c_Styles.size() == static_cast<size_t>(osc::MuscleSizingStyle::TOTAL));

    auto constexpr c_StyleStrings = osc::to_array<char const*>(
    {
        "OpenSim",
        "PCSA-derived",
    });
    static_assert(c_StyleStrings.size() == static_cast<size_t>(osc::MuscleSizingStyle::TOTAL));
}

nonstd::span<osc::MuscleSizingStyle const> osc::GetAllMuscleSizingStyles()
{
    return c_Styles;
}

nonstd::span<char const* const> osc::GetAllMuscleSizingStyleStrings()
{
    return c_StyleStrings;
}

ptrdiff_t osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
