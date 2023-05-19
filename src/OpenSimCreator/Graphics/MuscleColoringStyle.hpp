#pragma once

#include <nonstd/span.hpp>

namespace osc
{
    enum class MuscleColoringStyle {
        OpenSimAppearanceProperty,
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
    int GetIndexOf(MuscleColoringStyle);
}