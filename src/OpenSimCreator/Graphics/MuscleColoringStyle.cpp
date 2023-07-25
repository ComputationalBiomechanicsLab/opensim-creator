#include "MuscleColoringStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

namespace
{
    auto constexpr c_ColorStyles = osc::to_array<osc::MuscleColoringStyle>(
    {
        osc::MuscleColoringStyle::OpenSimAppearanceProperty,
        osc::MuscleColoringStyle::OpenSim,
        osc::MuscleColoringStyle::Activation,
        osc::MuscleColoringStyle::Excitation,
        osc::MuscleColoringStyle::Force,
        osc::MuscleColoringStyle::FiberLength,
    });
    static_assert(c_ColorStyles.size() == osc::NumOptions<osc::MuscleColoringStyle>());

    auto constexpr c_ColorStyleStrings = osc::to_array<char const*>(
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

nonstd::span<char const* const> osc::GetAllMuscleColoringStyleStrings()
{
    return c_ColorStyleStrings;
}

ptrdiff_t osc::GetIndexOf(osc::MuscleColoringStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
