#include "MuscleColorSource.h"

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleColorSourceMetadata>(
    {
        MuscleColorSourceMetadata
        {
            "opensim_appearance",
            "Appearance Property",
            MuscleColorSource::AppearanceProperty,
        },
        MuscleColorSourceMetadata
        {
            "activation",
            "Activation",
            MuscleColorSource::Activation,
        },
        MuscleColorSourceMetadata
        {
            "excitation",
            "Excitation",
            MuscleColorSource::Excitation,
        },
        MuscleColorSourceMetadata
        {
            "force",
            "Force",
            MuscleColorSource::Force,
        },
        MuscleColorSourceMetadata
        {
            "fiber_length",
            "Fiber Length",
            MuscleColorSource::FiberLength,
        },
    });
    static_assert(c_Metadata.size() == num_options<MuscleColorSource>());
}

std::span<const MuscleColorSourceMetadata> osc::GetAllPossibleMuscleColoringSourcesMetadata()
{
    return c_Metadata;
}

const MuscleColorSourceMetadata& osc::GetMuscleColoringStyleMetadata(MuscleColorSource s)
{
    return GetAllPossibleMuscleColoringSourcesMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(MuscleColorSource s)
{
    return static_cast<ptrdiff_t>(s);
}
