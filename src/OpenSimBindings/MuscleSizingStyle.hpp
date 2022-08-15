#pragma once

#include <nonstd/span.hpp>

namespace osc
{
    enum class MuscleSizingStyle {
        OpenSim = 0,
        SconePCSA,
        SconeNonPCSA,
        TOTAL,
        Default = OpenSim,
    };

    nonstd::span<MuscleSizingStyle const> GetAllMuscleSizingStyles();
    nonstd::span<char const* const> GetAllMuscleSizingStyleStrings();
    int GetIndexOf(MuscleSizingStyle);
}