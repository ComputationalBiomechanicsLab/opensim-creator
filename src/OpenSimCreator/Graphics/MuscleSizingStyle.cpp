#include "MuscleSizingStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    constexpr auto c_Styles = osc::to_array<osc::MuscleSizingStyle>(
    {
        osc::MuscleSizingStyle::OpenSim,
        osc::MuscleSizingStyle::PcsaDerived,
    });
    static_assert(c_Styles.size() == osc::NumOptions<osc::MuscleSizingStyle>());

    constexpr auto c_StyleStrings = osc::to_array<char const*>(
    {
        "OpenSim",
        "PCSA-derived",
    });
    static_assert(c_StyleStrings.size() == osc::NumOptions<osc::MuscleSizingStyle>());
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
