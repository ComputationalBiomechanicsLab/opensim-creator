#include "MuscleDecorationStyle.hpp"

#include <array>

static std::array<osc::MuscleDecorationStyle, static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL)> CreateDecorationStyleLut()
{
    return
    {
        osc::MuscleDecorationStyle::OpenSim,
        osc::MuscleDecorationStyle::Scone,
    };
}

static std::array<char const*, static_cast<size_t>(osc::MuscleDecorationStyle::TOTAL)> CreateDecorationStringsLut()
{
    return
    {
        "OpenSim",
        "Scone",
    };
}

nonstd::span<osc::MuscleDecorationStyle const> osc::GetAllMuscleDecorationStyles()
{
    static auto const g_Styles = CreateDecorationStyleLut();
    return g_Styles;
}

nonstd::span<char const* const> osc::GetAllMuscleDecorationStyleStrings()
{
    static auto const g_StyleStrings = CreateDecorationStringsLut();
    return g_StyleStrings;
}

int osc::GetIndexOf(MuscleDecorationStyle s)
{
    return static_cast<int>(s);
}
