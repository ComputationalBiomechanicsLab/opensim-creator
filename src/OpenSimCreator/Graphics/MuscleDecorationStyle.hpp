#pragma once

#include <nonstd/span.hpp>
#include <oscar/Utils/CStringView.hpp>

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

    struct MuscleDecorationStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleDecorationStyle value;
    };
    nonstd::span<MuscleDecorationStyleMetadata const> GetAllMuscleDecorationStyleMetadata();
    ptrdiff_t GetIndexOf(MuscleDecorationStyle);
    MuscleDecorationStyleMetadata const& GetMuscleDecorationStyleMetadata(MuscleDecorationStyle);
}
