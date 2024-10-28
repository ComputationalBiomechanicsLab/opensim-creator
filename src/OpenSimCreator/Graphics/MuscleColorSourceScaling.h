#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>

namespace osc
{
    enum class MuscleColorSourceScaling {
        None,
        ModelWide,
        NUM_OPTIONS,

        Default = None,
    };

    struct MuscleColorSourceScalingMetadata final {
        CStringView id;
        CStringView label;
        MuscleColorSourceScaling value;
    };

    std::span<const MuscleColorSourceScalingMetadata> GetAllPossibleMuscleColorSourceScalingMetadata();
    const MuscleColorSourceScalingMetadata& GetMuscleColorSourceScalingMetadata(MuscleColorSourceScaling);
    ptrdiff_t GetIndexOf(MuscleColorSourceScaling);
}
