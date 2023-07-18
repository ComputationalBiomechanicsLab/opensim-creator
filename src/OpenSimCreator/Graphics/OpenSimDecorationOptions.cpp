#include "OpenSimDecorationOptions.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>

#include <cstdint>
#include <type_traits>

namespace
{
    using CustomDecorationOptionFlags = uint32_t;
    enum CustomDecorationOptionFlags_ : uint32_t {
        CustomDecorationOptionFlags_None = 0,

        CustomDecorationOptionFlags_ShouldShowScapulo = 1<<0,
        CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForOrigin = 1<<1,
        CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForInsertion = 1<<2,
        CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForOrigin = 1<<3,
        CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForInsertion = 1<<4,
        CustomDecorationOptionFlags_ShouldShowCentersOfMass = 1<<5,
        CustomDecorationOptionFlags_ShouldShowPointToPointSprings = 1<<6,
        CustomDecorationOptionFlags_ShouldShowContactForces = 1<<7,

        CustomDecorationOptionFlags_COUNT = 8,
        CustomDecorationOptionFlags_Default =
            CustomDecorationOptionFlags_ShouldShowPointToPointSprings,
    };

    auto constexpr c_CustomDecorationOptionLabels = osc::to_array<osc::CStringView>(
    {
        "Scapulothoracic Joints",
        "Origin Lines of Action (effective)",
        "Insertion Lines of Action (effective)",
        "Origin Lines of Action (anatomical)",
        "Insertion Lines of Action (anatomical)",
        "Centers of Mass",
        "Point-to-Point Springs",
        "Plane Contact Forces (EXPERIMENTAL)",
    });
    static_assert(c_CustomDecorationOptionLabels.size() == CustomDecorationOptionFlags_COUNT);

    auto constexpr c_CustomDecorationDescriptions = osc::to_array<std::optional<osc::CStringView>>(
    {
        std::nullopt,
        "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        "Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        "Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!",
        std::nullopt,
        std::nullopt,
        "Tries to draw the direction of contact forces on planes in the scene.\n\nEXPERIMENTAL: the implementation of this visualization is work-in-progress and written by someone with a highschool-level understanding of Torque. Report any bugs or implementation opinions on GitHub.\n\nOpenSim Creator's implementation of this algorithm is very roughly based on Thomas Geijtenbeek's (better) implementation in scone-studio, here:\n\n    - https://github.com/tgeijten/scone-studio \n\nThanks to @tgeijten for writing an awesome project (that OSC has probably mis-implemented ;) - again, report any bugs, folks)",
    });
    static_assert(c_CustomDecorationDescriptions.size() == CustomDecorationOptionFlags_COUNT);

    void SetFlag(uint32_t& flags, uint32_t flag, bool v)
    {
        if (v)
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
        }
    }
}

osc::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    m_MuscleDecorationStyle{MuscleDecorationStyle::Default},
    m_MuscleColoringStyle{MuscleColoringStyle::Default},
    m_MuscleSizingStyle{MuscleSizingStyle::Default},
    m_Flags{CustomDecorationOptionFlags_Default}
{
    static_assert(std::is_same_v<std::underlying_type_t<CustomDecorationOptionFlags_>, std::decay_t<decltype(m_Flags)>>);
}

osc::MuscleDecorationStyle osc::OpenSimDecorationOptions::getMuscleDecorationStyle() const
{
    return m_MuscleDecorationStyle;
}

void osc::OpenSimDecorationOptions::setMuscleDecorationStyle(MuscleDecorationStyle s)
{
    m_MuscleDecorationStyle = s;
}

osc::MuscleColoringStyle osc::OpenSimDecorationOptions::getMuscleColoringStyle() const
{
    return m_MuscleColoringStyle;
}

void osc::OpenSimDecorationOptions::setMuscleColoringStyle(MuscleColoringStyle s)
{
    m_MuscleColoringStyle = s;
}

osc::MuscleSizingStyle osc::OpenSimDecorationOptions::getMuscleSizingStyle() const
{
    return m_MuscleSizingStyle;
}

void osc::OpenSimDecorationOptions::setMuscleSizingStyle(MuscleSizingStyle s)
{
    m_MuscleSizingStyle = s;
}

size_t osc::OpenSimDecorationOptions::getNumOptions() const
{
    return static_cast<size_t>(CustomDecorationOptionFlags_COUNT);
}

bool osc::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & (1<<i);
}

void osc::OpenSimDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetFlag(m_Flags, 1<<i, v);
}

osc::CStringView osc::OpenSimDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return c_CustomDecorationOptionLabels.at(i);
}

std::optional<osc::CStringView> osc::OpenSimDecorationOptions::getOptionDescription(ptrdiff_t i) const
{
    return c_CustomDecorationDescriptions.at(i);
}

bool osc::OpenSimDecorationOptions::getShouldShowScapulo() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowScapulo;
}

void osc::OpenSimDecorationOptions::setShouldShowScapulo(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowScapulo, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForOrigin() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForOrigin;
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForOrigin(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForInsertion() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForInsertion;
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForInsertion(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForOrigin() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForOrigin;
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForInsertion() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForInsertion;
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowCentersOfMass() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowCentersOfMass;
}

void osc::OpenSimDecorationOptions::setShouldShowCentersOfMass(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowCentersOfMass, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowPointToPointSprings() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowPointToPointSprings;
}

void osc::OpenSimDecorationOptions::setShouldShowPointToPointSprings(bool v)
{
    SetFlag(m_Flags, CustomDecorationOptionFlags_ShouldShowPointToPointSprings, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowContactForces() const
{
    return m_Flags & CustomDecorationOptionFlags_ShouldShowContactForces;
}

bool osc::operator==(OpenSimDecorationOptions const& a, OpenSimDecorationOptions const& b) noexcept
{
    return
        a.m_MuscleDecorationStyle == b.m_MuscleDecorationStyle &&
        a.m_MuscleColoringStyle == b.m_MuscleColoringStyle &&
        a.m_MuscleSizingStyle == b.m_MuscleSizingStyle &&
        a.m_Flags == b.m_Flags;
}
