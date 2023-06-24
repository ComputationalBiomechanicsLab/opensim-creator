#include "MuscleColoringStyle.hpp"

#include <oscar/Utils/ArrayHelpers.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <cstddef>

static auto constexpr c_ColorStyles = osc::MakeSizedArray<osc::MuscleColoringStyle, static_cast<size_t>(osc::MuscleColoringStyle::TOTAL)>
(
    osc::MuscleColoringStyle::OpenSimAppearanceProperty,
    osc::MuscleColoringStyle::OpenSim,
    osc::MuscleColoringStyle::Activation,
    osc::MuscleColoringStyle::Excitation,
    osc::MuscleColoringStyle::Force,
    osc::MuscleColoringStyle::FiberLength
);

static auto constexpr c_ColorStyleStrings = osc::MakeSizedArray<char const*, static_cast<size_t>(osc::MuscleColoringStyle::TOTAL)>
(
    "OpenSim (Appearance Property)",
    "OpenSim",
    "Activation",
    "Excitation",
    "Force",
    "Fiber Length"
);

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
