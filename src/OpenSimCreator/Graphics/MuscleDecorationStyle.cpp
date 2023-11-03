#include "MuscleDecorationStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace
{
    constexpr auto c_Metadata = osc::to_array<osc::MuscleDecorationStyleMetadata>(
    {
        osc::MuscleDecorationStyleMetadata
        {
            "opensim",
            "OpenSim",
            osc::MuscleDecorationStyle::OpenSim,
        },
        osc::MuscleDecorationStyleMetadata
        {
            "fibers_and_tendons",
            "Fibers & Tendons",
            osc::MuscleDecorationStyle::FibersAndTendons,
        },
        osc::MuscleDecorationStyleMetadata
        {
            "hidden",
            "Hidden",
            osc::MuscleDecorationStyle::Hidden,
        },
    });
    static_assert(c_Metadata.size() == osc::NumOptions<osc::MuscleDecorationStyle>());
}

std::span<osc::MuscleDecorationStyleMetadata const> osc::GetAllMuscleDecorationStyleMetadata()
{
    return c_Metadata;
}

ptrdiff_t osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<ptrdiff_t>(s);
}

osc::MuscleDecorationStyleMetadata const& osc::GetMuscleDecorationStyleMetadata(MuscleDecorationStyle s)
{
    return GetAllMuscleDecorationStyleMetadata()[GetIndexOf(s)];
}
