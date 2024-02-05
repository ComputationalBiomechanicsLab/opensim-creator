#include "MuscleDecorationStyle.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleDecorationStyleMetadata>(
    {
        MuscleDecorationStyleMetadata
        {
            "opensim",
            "OpenSim",
            MuscleDecorationStyle::OpenSim,
        },
        MuscleDecorationStyleMetadata
        {
            "fibers_and_tendons",
            "Fibers & Tendons",
            MuscleDecorationStyle::FibersAndTendons,
        },
        MuscleDecorationStyleMetadata
        {
            "hidden",
            "Hidden",
            MuscleDecorationStyle::Hidden,
        },
    });
    static_assert(c_Metadata.size() == NumOptions<MuscleDecorationStyle>());
}

std::span<MuscleDecorationStyleMetadata const> osc::GetAllMuscleDecorationStyleMetadata()
{
    return c_Metadata;
}

ptrdiff_t osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<ptrdiff_t>(s);
}

MuscleDecorationStyleMetadata const& osc::GetMuscleDecorationStyleMetadata(MuscleDecorationStyle s)
{
    return GetAllMuscleDecorationStyleMetadata()[GetIndexOf(s)];
}
