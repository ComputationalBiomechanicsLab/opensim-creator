#include "MuscleDecorationStyle.h"

#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleDecorationStyleMetadata>({
        MuscleDecorationStyleMetadata{
            "opensim",  // legacy label (naming was changed to "Lines of Action" in #933)
            "Lines of Action",
            MuscleDecorationStyle::LinesOfAction,
        },
        MuscleDecorationStyleMetadata{
            "fibers_and_tendons",
            "Fibers & Tendons",
            MuscleDecorationStyle::FibersAndTendons,
        },
        MuscleDecorationStyleMetadata{
            "hidden",
            "Hidden",
            MuscleDecorationStyle::Hidden,
        },
    });
    static_assert(c_Metadata.size() == num_options<MuscleDecorationStyle>());
}

std::span<const MuscleDecorationStyleMetadata> osc::GetAllMuscleDecorationStyleMetadata()
{
    return c_Metadata;
}

ptrdiff_t osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<ptrdiff_t>(s);
}

const MuscleDecorationStyleMetadata& osc::GetMuscleDecorationStyleMetadata(MuscleDecorationStyle s)
{
    return GetAllMuscleDecorationStyleMetadata()[GetIndexOf(s)];
}
