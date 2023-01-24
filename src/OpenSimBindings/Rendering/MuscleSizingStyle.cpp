#include "MuscleSizingStyle.hpp"

#include "src/Utils/Algorithms.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

static constexpr auto c_Styles = osc::MakeSizedArray<osc::MuscleSizingStyle, static_cast<size_t>(osc::MuscleSizingStyle::TOTAL)>
(
    osc::MuscleSizingStyle::OpenSim,
    osc::MuscleSizingStyle::PcsaDerived
);

static constexpr auto c_StyleStrings = osc::MakeSizedArray<char const*, static_cast<size_t>(osc::MuscleSizingStyle::TOTAL)>
(
    "OpenSim",
    "PCSA-derived"
);

nonstd::span<osc::MuscleSizingStyle const> osc::GetAllMuscleSizingStyles()
{
    return c_Styles;
}

nonstd::span<char const* const> osc::GetAllMuscleSizingStyleStrings()
{
    return c_StyleStrings;
}

int osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<int>(s);
}
