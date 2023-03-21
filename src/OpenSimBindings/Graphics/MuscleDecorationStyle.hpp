#pragma once

#include <nonstd/span.hpp>

namespace osc
{
    enum class MuscleDecorationStyle {
        OpenSim = 0,
        FibersAndTendons,
        Hidden,
        TOTAL,
        Default = OpenSim,
    };

    nonstd::span<MuscleDecorationStyle const> GetAllMuscleDecorationStyles();
    nonstd::span<char const* const> GetAllMuscleDecorationStyleStrings();
    int GetIndexOf(MuscleDecorationStyle);
}