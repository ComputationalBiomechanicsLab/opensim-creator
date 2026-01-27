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

    std::span<const MuscleColorSourceScalingMetadata> GetAllPossibleMuscleColorSourceScalingMetadata();
    const MuscleColorSourceScalingMetadata& GetMuscleColorSourceScalingMetadata(MuscleColorSourceScaling);
    ptrdiff_t GetIndexOf(MuscleColorSourceScaling);
}
