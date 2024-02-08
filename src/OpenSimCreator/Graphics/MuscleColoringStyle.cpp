#include "MuscleColoringStyle.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <cstddef>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_Metadata = std::to_array<MuscleColoringStyleMetadata>(
    {
        MuscleColoringStyleMetadata
        {
            "opensim_appearance",
            "OpenSim (Appearance Property)",
            MuscleColoringStyle::OpenSimAppearanceProperty,
        },
        MuscleColoringStyleMetadata
        {
            "opensim",
            "OpenSim",
            MuscleColoringStyle::OpenSim,
        },
        MuscleColoringStyleMetadata
        {
            "activation",
            "Activation",
            MuscleColoringStyle::Activation,
        },
        MuscleColoringStyleMetadata
        {
            "excitation",
            "Excitation",
            MuscleColoringStyle::Excitation,
        },
        MuscleColoringStyleMetadata
        {
            "force",
            "Force",
            MuscleColoringStyle::Force,
        },
        MuscleColoringStyleMetadata
        {
            "fiber_length",
            "Fiber Length",
            MuscleColoringStyle::FiberLength,
        },
    });
    static_assert(c_Metadata.size() == NumOptions<MuscleColoringStyle>());
}

std::span<MuscleColoringStyleMetadata const> osc::GetAllMuscleColoringStyleMetadata()
{
    return c_Metadata;
}

MuscleColoringStyleMetadata const& osc::GetMuscleColoringStyleMetadata(MuscleColoringStyle s)
{
    return GetAllMuscleColoringStyleMetadata()[GetIndexOf(s)];
}

ptrdiff_t osc::GetIndexOf(MuscleColoringStyle s)
{
    return static_cast<ptrdiff_t>(s);
}
