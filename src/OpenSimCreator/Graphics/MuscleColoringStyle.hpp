#pragma once

#include <nonstd/span.hpp>

#include <cstddef>

namespace osc
{
    enum class MuscleColoringStyle {
        OpenSimAppearanceProperty = 0,
        OpenSim,
        Activation,
        Excitation,
        Force,
        FiberLength,
        TOTAL,
        Default = OpenSim,
    };

    nonstd::span<MuscleColoringStyle const> GetAllMuscleColoringStyles();
    nonstd::span<char const* const> GetAllMuscleColoringStyleStrings();
    ptrdiff_t GetIndexOf(MuscleColoringStyle);
}