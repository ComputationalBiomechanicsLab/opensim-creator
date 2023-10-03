#include "OpenSimDecorationOptionFlags.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>

#include <algorithm>

constexpr auto c_CustomDecorationOptionLabels = osc::to_array<osc::OpenSimDecorationOptionMetadata>(
{
    osc::OpenSimDecorationOptionMetadata
    {
        "Scapulothoracic Joints",
        std::nullopt,
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Origin Lines of Action (effective)",
        "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Insertion Lines of Action (effective)",
        "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Origin Lines of Action (anatomical)",
        "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Insertion Lines of Action (anatomical)",
        "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Centers of Mass",
        std::nullopt,
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Point-to-Point Springs",
        std::nullopt,
    },
    osc::OpenSimDecorationOptionMetadata
    {
        "Plane Contact Forces (EXPERIMENTAL)",
        "Tries to draw the direction of contact forces on planes in the scene.\n\nEXPERIMENTAL: the implementation of this visualization is work-in-progress and written by someone with a highschool-level understanding of Torque. Report any bugs or implementation opinions on GitHub.\n\nOpenSim Creator's implementation of this algorithm is very roughly based on Thomas Geijtenbeek's (better) implementation in scone-studio, here:\n\n    - https://github.com/tgeijten/scone-studio \n\nThanks to @tgeijten for writing an awesome project (that OSC has probably mis-implemented ;) - again, report any bugs, folks)",
    }
});

osc::OpenSimDecorationOptionMetadata const& osc::GetIthOptionMetadata(size_t i)
{
    return c_CustomDecorationOptionLabels.at(i);
}

osc::OpenSimDecorationOptionFlags osc::GetIthOption(size_t i)
{
    auto v = 1u << std::min(i, NumOptions<OpenSimDecorationOptionFlags>()-1);
    return static_cast<OpenSimDecorationOptionFlags>(v);
}

void osc::SetIthOption(OpenSimDecorationOptionFlags& flags, size_t i, bool v)
{
    SetOption(flags, GetIthOption(i), v);
}

void osc::SetOption(OpenSimDecorationOptionFlags& flags, OpenSimDecorationOptionFlags flag, bool v)
{
    using Underlying = std::underlying_type_t<OpenSimDecorationOptionFlags>;

    if (v)
    {
        flags = static_cast<OpenSimDecorationOptionFlags>(static_cast<Underlying>(flags) | static_cast<Underlying>(flag));
    }
    else
    {
        flags = static_cast<OpenSimDecorationOptionFlags>(static_cast<Underlying>(flags) & ~static_cast<Underlying>(flag));
    }
}
