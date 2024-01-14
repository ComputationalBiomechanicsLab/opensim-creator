#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleDecorationStyle {
        OpenSim,
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
    std::span<MuscleDecorationStyleMetadata const> GetAllMuscleDecorationStyleMetadata();
    ptrdiff_t GetIndexOf(MuscleDecorationStyle);
    MuscleDecorationStyleMetadata const& GetMuscleDecorationStyleMetadata(MuscleDecorationStyle);
}
