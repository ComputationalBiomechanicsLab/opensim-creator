#include "MuscleColoringStyle.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace
{
    constexpr auto c_Metadata = osc::to_array<osc::MuscleColoringStyleMetadata>(
    {
        osc::MuscleColoringStyleMetadata
        {
            "opensim_appearance",
            "OpenSim (Appearance Property)",
            osc::MuscleColoringStyle::OpenSimAppearanceProperty,
        },
        osc::MuscleColoringStyleMetadata
        {
            "opensim",
            "OpenSim",
            osc::MuscleColoringStyle::OpenSim,
        },
        osc::MuscleColoringStyleMetadata
        {
            "activation",
            "Activation",
            osc::MuscleColoringStyle::Activation,
        },
        osc::MuscleColoringStyleMetadata
        {
            "excitation",
            "Excitation",
            osc::MuscleColoringStyle::Excitation,
        },
        osc::MuscleColoringStyleMetadata
        {
            "force",
            "Force",
            osc::MuscleColoringStyle::Force,
        },
        osc::MuscleColoringStyleMetadata
        {
            "fiber_length",
            "Fiber Length",
            osc::MuscleColoringStyle::FiberLength,
        },
    });
    static_assert(c_Metadata.size() == osc::NumOptions<osc::MuscleColoringStyle>());
}

std::span<osc::MuscleColoringStyleMetadata const> osc::GetAllMuscleColoringStyleMetadata()
{
    return c_Metadata;
}

osc::MuscleColoringStyleMetadata const& osc::GetMuscleColoringStyleMetadata(MuscleColoringStyle s)
{
    return GetAllMuscleColoringStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(osc::MuscleColoringStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
