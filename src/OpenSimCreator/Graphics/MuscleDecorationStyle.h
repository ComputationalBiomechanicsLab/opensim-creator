#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleDecorationStyle {
        LinesOfAction,
        FibersAndTendons,
        Hidden,
        NUM_OPTIONS,

        Default = LinesOfAction,
    };

    struct MuscleDecorationStyleMetadata final {
        CStringView id;
        CStringView label;
        MuscleDecorationStyle value;
    };
    std::span<const MuscleDecorationStyleMetadata> GetAllMuscleDecorationStyleMetadata();
    ptrdiff_t GetIndexOf(MuscleDecorationStyle);
    const MuscleDecorationStyleMetadata& GetMuscleDecorationStyleMetadata(MuscleDecorationStyle);
}
