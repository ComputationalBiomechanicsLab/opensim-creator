#include "muscle_color_source_scaling.h"

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace opyn;

namespace
{
    constexpr auto c_metadata = std::to_array<MuscleColorSourceScalingMetadata>({
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
    static_assert(c_metadata.size() == osc::num_options<MuscleColorSourceScaling>());
}

std::span<const MuscleColorSourceScalingMetadata> opyn::get_all_possible_muscle_color_source_scaling_metadata()
{
    return c_metadata;
}

const MuscleColorSourceScalingMetadata& opyn::get_muscle_color_source_scaling_metadata(MuscleColorSourceScaling option)
{
    return c_metadata.at(osc::to_index(option));
}

ptrdiff_t opyn::get_index_of(MuscleColorSourceScaling s)
{
    return static_cast<ptrdiff_t>(s);
}
