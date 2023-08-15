#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>

#include <cstddef>

namespace osc
{
    enum class MuscleDecorationStyle {
        OpenSim = 0,
        FibersAndTendons,
        Hidden,
        NUM_OPTIONS,

        Default = OpenSim,
    };

    nonstd::span<MuscleDecorationStyle const> GetAllMuscleDecorationStyles();
    nonstd::span<CStringView const> GetAllMuscleDecorationStyleStrings();
    ptrdiff_t GetIndexOf(MuscleDecorationStyle);
}
