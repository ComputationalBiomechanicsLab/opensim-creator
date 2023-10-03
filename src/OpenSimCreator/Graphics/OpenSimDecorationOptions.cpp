#include "OpenSimDecorationOptions.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <optional>

osc::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    m_MuscleDecorationStyle{MuscleDecorationStyle::Default},
    m_MuscleColoringStyle{MuscleColoringStyle::Default},
    m_MuscleSizingStyle{MuscleSizingStyle::Default},
    m_Flags{OpenSimDecorationOptionFlags::Default}
{
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
    return NumOptions<OpenSimDecorationOptionFlags>();
}

bool osc::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & GetIthOption(i);
}

void osc::OpenSimDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetIthOption(m_Flags, i, v);
}

osc::CStringView osc::OpenSimDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).label;
}

std::optional<osc::CStringView> osc::OpenSimDecorationOptions::getOptionDescription(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).maybeDescription;
}

bool osc::OpenSimDecorationOptions::getShouldShowScapulo() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowScapulo;
}

void osc::OpenSimDecorationOptions::setShouldShowScapulo(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowScapulo, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForOrigin() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowEffectiveLinesOfActionForOrigin;
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForOrigin(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForInsertion() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowEffectiveLinesOfActionForInsertion;
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForInsertion(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForOrigin() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowAnatomicalMuscleLinesOfActionForOrigin;
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForInsertion() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowAnatomicalMuscleLinesOfActionForInsertion;
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowCentersOfMass() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowCentersOfMass;
}

void osc::OpenSimDecorationOptions::setShouldShowCentersOfMass(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowCentersOfMass, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowPointToPointSprings() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowPointToPointSprings;
}

void osc::OpenSimDecorationOptions::setShouldShowPointToPointSprings(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowPointToPointSprings, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowContactForces() const
{
    return m_Flags & OpenSimDecorationOptionFlags::ShouldShowContactForces;
}

bool osc::operator==(OpenSimDecorationOptions const& a, OpenSimDecorationOptions const& b) noexcept
{
    return
        a.m_MuscleDecorationStyle == b.m_MuscleDecorationStyle &&
        a.m_MuscleColoringStyle == b.m_MuscleColoringStyle &&
        a.m_MuscleSizingStyle == b.m_MuscleSizingStyle &&
        a.m_Flags == b.m_Flags;
}
