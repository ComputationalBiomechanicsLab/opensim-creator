#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <span>

namespace opyn
{
    enum class MuscleDecorationStyle {
        LinesOfAction,
        FibersAndTendons,
        Hidden,
        NUM_OPTIONS,

        Default = LinesOfAction,
    };

    struct MuscleDecorationStyleMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        MuscleDecorationStyle value;
    };
    std::span<const MuscleDecorationStyleMetadata> GetAllMuscleDecorationStyleMetadata();
    ptrdiff_t GetIndexOf(MuscleDecorationStyle);
    const MuscleDecorationStyleMetadata& GetMuscleDecorationStyleMetadata(MuscleDecorationStyle);
}
