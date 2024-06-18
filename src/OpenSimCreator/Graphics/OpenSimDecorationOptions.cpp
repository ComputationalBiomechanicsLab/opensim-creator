#include "OpenSimDecorationOptions.h"

#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <optional>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

osc::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    m_MuscleDecorationStyle{MuscleDecorationStyle::Default},
    m_MuscleColoringStyle{MuscleColoringStyle::Default},
    m_MuscleSizingStyle{MuscleSizingStyle::Default},
    m_Flags{OpenSimDecorationOptionFlags::Default}
{}

MuscleDecorationStyle osc::OpenSimDecorationOptions::getMuscleDecorationStyle() const
{
    return m_MuscleDecorationStyle;
}

void osc::OpenSimDecorationOptions::setMuscleDecorationStyle(MuscleDecorationStyle s)
{
    m_MuscleDecorationStyle = s;
}

MuscleColoringStyle osc::OpenSimDecorationOptions::getMuscleColoringStyle() const
{
    return m_MuscleColoringStyle;
}

void osc::OpenSimDecorationOptions::setMuscleColoringStyle(MuscleColoringStyle s)
{
    m_MuscleColoringStyle = s;
}

MuscleSizingStyle osc::OpenSimDecorationOptions::getMuscleSizingStyle() const
{
    return m_MuscleSizingStyle;
}

void osc::OpenSimDecorationOptions::setMuscleSizingStyle(MuscleSizingStyle s)
{
    m_MuscleSizingStyle = s;
}

size_t osc::OpenSimDecorationOptions::getNumOptions() const
{
    return num_flags<OpenSimDecorationOptionFlags>();
}

bool osc::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags & GetIthOption(i);
}

void osc::OpenSimDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetIthOption(m_Flags, i, v);
}

CStringView osc::OpenSimDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).label;
}

std::optional<CStringView> osc::OpenSimDecorationOptions::getOptionDescription(ptrdiff_t i) const
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

void osc::OpenSimDecorationOptions::setShouldShowContactForces(bool v)
{
    SetOption(m_Flags, OpenSimDecorationOptionFlags::ShouldShowContactForces, v);
}

void osc::OpenSimDecorationOptions::forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const AppSettingValue&)>& callback) const
{
    callback("muscle_decoration_style", AppSettingValue{GetMuscleDecorationStyleMetadata(m_MuscleDecorationStyle).id});
    callback("muscle_coloring_style", AppSettingValue{GetMuscleColoringStyleMetadata(m_MuscleColoringStyle).id});
    callback("muscle_sizing_style", AppSettingValue{GetMuscleSizingStyleMetadata(m_MuscleSizingStyle).id});
    for (size_t i = 0; i < num_flags<OpenSimDecorationOptionFlags>(); ++i)
    {
        const auto& meta = GetIthOptionMetadata(i);
        const bool v = m_Flags & GetIthOption(i);
        callback(meta.id, AppSettingValue{v});
    }
}

void osc::OpenSimDecorationOptions::tryUpdFromValues(
    std::string_view prefix,
    const std::unordered_map<std::string, AppSettingValue>& lut)
{
    // looks up a single element in the lut
    auto lookup = [
        &lut,
        buf = std::string{prefix},
        prefixLen = prefix.size()](std::string_view v) mutable
    {
        buf.resize(prefixLen);
        buf.insert(prefixLen, v);

        return lookup_or_nullptr(lut, buf);
    };

    if (auto* appVal = lookup("muscle_decoration_style"); appVal->type() == AppSettingValueType::String)
    {
        const auto metadata = GetAllMuscleDecorationStyleMetadata();
        const auto it = rgs::find(metadata, appVal->to_string(), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleDecorationStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_coloring_style"); appVal->type() == AppSettingValueType::String)
    {
        const auto metadata = GetAllMuscleColoringStyleMetadata();
        const auto it = rgs::find(metadata, appVal->to_string(), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleColoringStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_sizing_style"); appVal->type() == AppSettingValueType::String)
    {
        const auto metadata = GetAllMuscleSizingStyleMetadata();
        const auto it = rgs::find(metadata, appVal->to_string(), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleSizingStyle = it->value;
        }
    }

    for (size_t i = 0; i < num_flags<OpenSimDecorationOptionFlags>(); ++i)
    {
        const auto& metadata = GetIthOptionMetadata(i);
        if (auto* appVal = lookup(metadata.id); appVal->type() == AppSettingValueType::Bool)
        {
            const bool v = appVal->to_bool();
            SetOption(m_Flags, GetIthOption(i), v);
        }
    }
}
