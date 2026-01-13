#include "OpenSimDecorationOptionFlags.h"

#include <libopensimcreator/Platform/msmicons.h>

#include <liboscar/utils/Algorithms.h>
#include <liboscar/utils/EnumHelpers.h>

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
            "Effective Origin Lines of Action",
            "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_insertion_effective_line_of_action",
            "Effective Insertion Lines of Action",
            "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_origin_anatomical_line_of_action",
            "Anatomical Origin Lines of Action",
            "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_muscle_insertion_anatomical_line_of_action",
            "Anatomical Insertion Lines of Action",
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
            "Plane Contact Forces (experimental)",
            "Tries to draw the direction of contact forces on planes in the scene.\n\nEXPERIMENTAL: the implementation of this visualization is work-in-progress and written by someone with a highschool-log_level_ understanding of Torque. Report any bugs or implementation opinions on GitHub.\n\nOpenSim Creator's implementation of this algorithm is very roughly based on Thomas Geijtenbeek's (better) implementation in scone-studio, here:\n\n    - https://github.com/tgeijten/scone-studio \n\nThanks to @tgeijten for writing an awesome project (that OSC has probably mis-implemented ;) - again, report any bugs, folks)",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_force_linear_component",
            "Forces on Bodies (experimental)",
            "Tries to draw the linear component applied by each `OpenSim::Force` in the model.\n\nEXPERIMENTAL: this currently iterates through all the forces and extracts their linear component w.r.t. the body frame, it's probably slow, and probably noisy, but also probably still useful to know (e.g. if you're debugging weird model behavior)",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_force_angular_component",
            "Torques on Bodies (experimental)",
            "Tries to draw the angular component applied by each `OpenSim::Force` in the model.\n\nEXPERIMENTAL: this currently iterates through all the forces and extracts their angular component w.r.t. the body frame, it's probably slow, and probably noisy, but also probably still useful to know (e.g. if you're debugging weird model behavior)",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_point_forces",
            "Point Forces (experimental)",
            "Tries to draw the an arrow to the point where point-based linear force component(s) are applied. This only applies to `OpenSim::Force`s that support applying forces to points.\n\nEXPERIMENTAL: for technical reasons, this implementation is ad-hoc: it currently only works for `ExternalForce`s and `GeometryPath`s",
        },
        OpenSimDecorationOptionMetadata
        {
            "show_scholz_2015_obstacle_contact_hints",
            "Scholz Obstacle Contact Hints",
            "Draws a sphere where at the `contact_hint` location for each `OpenSim::Scholz2015GeometryPathObstacle` in the model",
        },
    });

    static_assert(c_CustomDecorationOptionLabels.size() == num_flags<OpenSimDecorationOptionFlag>());
}


const OpenSimDecorationOptionMetadata& osc::GetIthOptionMetadata(size_t i)
{
    return c_CustomDecorationOptionLabels.at(i);
}

OpenSimDecorationOptionFlag osc::GetIthOption(size_t i)
{
    auto v = 1u << min(i, num_flags<OpenSimDecorationOptionFlag>()-1);
    return static_cast<OpenSimDecorationOptionFlag>(v);
}

void osc::SetIthOption(OpenSimDecorationOptionFlags& flags, size_t i, bool v)
{
    flags.set(GetIthOption(i), v);
}
