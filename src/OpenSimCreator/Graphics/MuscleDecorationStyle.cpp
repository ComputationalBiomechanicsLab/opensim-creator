#include "MuscleDecorationStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    constexpr auto c_Styles = osc::to_array<osc::MuscleDecorationStyle>(
    {
        osc::MuscleDecorationStyle::OpenSim,
        osc::MuscleDecorationStyle::FibersAndTendons,
        osc::MuscleDecorationStyle::Hidden,
    });
    static_assert(c_Styles.size() == osc::NumOptions<osc::MuscleDecorationStyle>());

    constexpr auto c_StyleStrings = osc::to_array<osc::CStringView>(
    {
        "OpenSim",
        "Fibers & Tendons",
        "Hidden",
    });
    static_assert(c_StyleStrings.size() == osc::NumOptions<osc::MuscleDecorationStyle>());
}

nonstd::span<osc::MuscleDecorationStyle const> osc::GetAllMuscleDecorationStyles()
{
    return c_Styles;
}

nonstd::span<osc::CStringView const> osc::GetAllMuscleDecorationStyleStrings()
{
    return c_StyleStrings;
}

ptrdiff_t osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
