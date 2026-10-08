#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <span>

namespace opyn
{
    enum class MuscleColorSourceScaling {
        None,
        ModelWide,
        NUM_OPTIONS,

        Default = None,
    };

    struct MuscleColorSourceScalingMetadata final {
        osc::CStringView id;
        osc::CStringView label;
        MuscleColorSourceScaling value;
    };

    std::span<const MuscleColorSourceScalingMetadata> get_all_possible_muscle_color_source_scaling_metadata();
    const MuscleColorSourceScalingMetadata& get_muscle_color_source_scaling_metadata(MuscleColorSourceScaling);
    ptrdiff_t get_index_of(MuscleColorSourceScaling);
}
