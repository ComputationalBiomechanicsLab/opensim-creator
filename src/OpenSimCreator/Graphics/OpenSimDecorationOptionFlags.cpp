#include "OpenSimDecorationOptionFlags.h"

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/EnumHelpers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

using namespace osc;

namespace
{
    constexpr auto c_CustomDecorationOptionLabels = std::to_array<OpenSimDecorationOptionMetadata>(
    {
        OpenSimDecorationOptionMetadata
        {
            "should_show_scapulo",
            "Scapulothoracic Joints",
            std::nullopt,
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_origin_effective_line_of_action",
            "Origin Lines of Action (effective)",
            "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_insertion_effective_line_of_action",
            "Insertion Lines of Action (effective)",
            "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_origin_anatomical_line_of_action",
            "Origin Lines of Action (anatomical)",
            "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_insertion_anatomical_line_of_action",
            "Insertion Lines of Action (anatomical)",
            "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_centers_of_mass",
            "Centers of Mass",
            std::nullopt,
        },
        OpenSimDecorationOptionMetadata
        {
            "show_point_to_point_springs",
            "Point-to-Point Springs",
            std::nullopt,
        },
        OpenSimDecorationOptionMetadata
        {
            "show_contact_forces",
            "Plane Contact Forces (EXPERIMENTAL)",
            "Tries to draw the direction of contact forces on planes in the scene.\n\nEXPERIMENTAL: the implementation of this visualization is work-in-progress and written by someone with a highschool-log_level_ understanding of Torque. Report any bugs or implementation opinions on GitHub.\n\nOpenSim Creator's implementation of this algorithm is very roughly based on Thomas Geijtenbeek's (better) implementation in scone-studio, here:\n\n    - https://github.com/tgeijten/scone-studio \n\nThanks to @tgeijten for writing an awesome project (that OSC has probably mis-implemented ;) - again, report any bugs, folks)",
        },
    });
}


OpenSimDecorationOptionMetadata const& osc::GetIthOptionMetadata(size_t i)
{
    return c_CustomDecorationOptionLabels.at(i);
}

OpenSimDecorationOptionFlags osc::GetIthOption(size_t i)
{
    auto v = 1u << min(i, NumFlags<OpenSimDecorationOptionFlags>()-1);
    return static_cast<OpenSimDecorationOptionFlags>(v);
}

void osc::SetIthOption(OpenSimDecorationOptionFlags& flags, size_t i, bool v)
{
    SetOption(flags, GetIthOption(i), v);
}

void osc::SetOption(OpenSimDecorationOptionFlags& flags, OpenSimDecorationOptionFlags flag, bool v)
{
    if (v)
    {
        flags = static_cast<OpenSimDecorationOptionFlags>(cpp23::to_underlying(flags) | cpp23::to_underlying(flag));
    }
    else
    {
        flags = static_cast<OpenSimDecorationOptionFlags>(cpp23::to_underlying(flags) & ~cpp23::to_underlying(flag));
    }
}
