#include "MuscleColoringStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    constexpr auto c_ColorStyles = osc::to_array<osc::MuscleColoringStyle>(
    {
        osc::MuscleColoringStyle::OpenSimAppearanceProperty,
        osc::MuscleColoringStyle::OpenSim,
        osc::MuscleColoringStyle::Activation,
        osc::MuscleColoringStyle::Excitation,
        osc::MuscleColoringStyle::Force,
        osc::MuscleColoringStyle::FiberLength,
    });
    static_assert(c_ColorStyles.size() == osc::NumOptions<osc::MuscleColoringStyle>());

    constexpr auto c_ColorStyleStrings = osc::to_array<osc::CStringView>(
    {
        "OpenSim (Appearance Property)",
        "OpenSim",
        "Activation",
        "Excitation",
        "Force",
        "Fiber Length",
    });
    static_assert(c_ColorStyleStrings.size() == osc::NumOptions<osc::MuscleColoringStyle>());
}

nonstd::span<osc::MuscleColoringStyle const> osc::GetAllMuscleColoringStyles()
{
    return c_ColorStyles;
}

nonstd::span<osc::CStringView const> osc::GetAllMuscleColoringStyleStrings()
{
    return c_ColorStyleStrings;
}

ptrdiff_t osc::GetIndexOf(osc::MuscleColoringStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
