#include "MuscleSizingStyle.hpp"

#include <array>

static std::array<osc::MuscleSizingStyle, static_cast<size_t>(osc::MuscleSizingStyle::TOTAL)> CreateDecorationStyleLut()
{
    return
    {
        osc::MuscleSizingStyle::OpenSim,
        osc::MuscleSizingStyle::SconePCSA,
        osc::MuscleSizingStyle::SconeNonPCSA,
    };
}

static std::array<char const*, static_cast<size_t>(osc::MuscleSizingStyle::TOTAL)> CreateDecorationStringsLut()
{
    return
    {
        "OpenSim (fixed volume)",
        "SCONE (PCSA derived)",
        "SCONE (alt, fixed volume)",
    };
}

nonstd::span<osc::MuscleSizingStyle const> osc::GetAllMuscleSizingStyles()
{
    static auto const g_Styles = CreateDecorationStyleLut();
    return g_Styles;
}

nonstd::span<char const* const> osc::GetAllMuscleSizingStyleStrings()
{
    static auto const g_StyleStrings = CreateDecorationStringsLut();
    return g_StyleStrings;
}

int osc::GetIndexOf(MuscleSizingStyle s)
{
    return static_cast<int>(s);
}