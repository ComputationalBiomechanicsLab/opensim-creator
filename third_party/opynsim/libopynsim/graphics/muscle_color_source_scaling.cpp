#include "muscle_color_source_scaling.h"

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace opyn;

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
    static_assert(c_Metadata.size() == osc::num_options<MuscleColorSourceScaling>());
}

std::span<const MuscleColorSourceScalingMetadata> opyn::GetAllPossibleMuscleColorSourceScalingMetadata()
{
    return c_Metadata;
}

const MuscleColorSourceScalingMetadata& opyn::GetMuscleColorSourceScalingMetadata(MuscleColorSourceScaling option)
{
    return c_Metadata.at(osc::to_index(option));
}

ptrdiff_t opyn::GetIndexOf(MuscleColorSourceScaling s)
{
    return static_cast<ptrdiff_t>(s);
}
