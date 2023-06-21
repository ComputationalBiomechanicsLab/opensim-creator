#include "MuscleDecorationStyle.hpp"

#include <oscar/Utils/Algorithms.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

static auto constexpr c_Styles = osc::MakeSizedArray<osc::MuscleDecorationStyle, static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL)>
(
    osc::MuscleDecorationStyle::OpenSim,
    osc::MuscleDecorationStyle::FibersAndTendons,
    osc::MuscleDecorationStyle::Hidden
);

static auto constexpr c_StyleStrings = osc::MakeSizedArray<char const*, static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL)>
(
    "OpenSim",
    "Fibers & Tendons",
    "Hidden"
);

nonstd::span<osc::MuscleDecorationStyle const> osc::GetAllMuscleDecorationStyles()
{
    return c_Styles;
}

nonstd::span<char const* const> osc::GetAllMuscleDecorationStyleStrings()
{
    return c_StyleStrings;
}

ptrdiff_t osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
