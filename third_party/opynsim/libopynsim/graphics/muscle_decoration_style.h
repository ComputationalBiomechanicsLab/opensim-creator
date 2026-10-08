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
    std::span<const MuscleDecorationStyleMetadata> get_all_muscle_decoration_style_metadata();
    ptrdiff_t get_index_of(MuscleDecorationStyle);
    const MuscleDecorationStyleMetadata& get_muscle_decoration_style_metadata(MuscleDecorationStyle);
}
