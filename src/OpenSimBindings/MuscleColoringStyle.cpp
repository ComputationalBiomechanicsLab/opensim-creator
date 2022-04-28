#include "MuscleColoringStyle.hpp"

#include <array>

static std::array<osc::MuscleColoringStyle, static_cast<size_t>(osc::MuscleColoringStyle::TOTAL)> CreateDecorationStyleLut()
{
    return
    {
        osc::MuscleColoringStyle::OpenSim,
        osc::MuscleColoringStyle::Activation,
        osc::MuscleColoringStyle::Excitation,
        osc::MuscleColoringStyle::Force,
        osc::MuscleColoringStyle::FiberLength,
    };
}

static std::array<char const*, static_cast<size_t>(osc::MuscleColoringStyle::TOTAL)> CreateDecorationStringsLut()
{
    return
    {
        "OpenSim",
        "Activation",
        "Excitation",
        "Force",
        "Fiber Length",
    };
}

nonstd::span<osc::MuscleColoringStyle const> osc::GetAllMuscleColoringStyles()
{
    static auto const g_ColorStyles = CreateDecorationStyleLut();
    return g_ColorStyles;
}

nonstd::span<char const* const> osc::GetAllMuscleColoringStyleStrings()
{
    static auto const g_ColorStyleStrings = CreateDecorationStringsLut();
    return g_ColorStyleStrings;
}

int osc::GetIndexOf(osc::MuscleColoringStyle s)
{
    return static_cast<int>(s);
}