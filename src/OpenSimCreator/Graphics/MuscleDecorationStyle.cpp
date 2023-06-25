#include "MuscleDecorationStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    auto constexpr c_Styles = osc::to_array<osc::MuscleDecorationStyle>(
    {
        osc::MuscleDecorationStyle::OpenSim,
        osc::MuscleDecorationStyle::FibersAndTendons,
        osc::MuscleDecorationStyle::Hidden,
    });
    static_assert(c_Styles.size() == static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL));

    auto constexpr c_StyleStrings = osc::to_array<char const*>(
    {
        "OpenSim",
        "Fibers & Tendons",
        "Hidden",
    });
    static_assert(c_StyleStrings.size() == static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL));
}

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
