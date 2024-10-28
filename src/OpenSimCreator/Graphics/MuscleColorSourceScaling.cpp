#include "MuscleColorSourceScaling.h"

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleColorSourceScalingMetadata>({
        MuscleColorSourceScalingMetadata{
            "none",
            "None",
            MuscleColorSourceScaling::None,
        },
        MuscleColorSourceScalingMetadata{
            "model_wide",
            "Model-Wide",
            MuscleColorSourceScaling::ModelWide,
        },
    });
    static_assert(c_Metadata.size() == num_options<MuscleColorSourceScaling>());
}

std::span<const MuscleColorSourceScalingMetadata> osc::GetAllPossibleMuscleColorSourceScalingMetadata()
{
    return c_Metadata;
}

const MuscleColorSourceScalingMetadata& osc::GetMuscleColorSourceScalingMetadata(MuscleColorSourceScaling option)
{
    return c_Metadata.at(to_index(option));
}

ptrdiff_t osc::GetIndexOf(MuscleColorSourceScaling s)
{
    return static_cast<ptrdiff_t>(s);
}
