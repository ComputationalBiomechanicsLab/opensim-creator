#pragma once

#include <nonstd/span.hpp>

#include <cstddef>

namespace osc
{
    enum class MuscleSizingStyle {
        OpenSim = 0,
        PcsaDerived,
        NUM_OPTIONS,

        Default = OpenSim,
    };

    nonstd::span<MuscleSizingStyle const> GetAllMuscleSizingStyles();
    nonstd::span<char const* const> GetAllMuscleSizingStyleStrings();
    ptrdiff_t GetIndexOf(MuscleSizingStyle);
}
