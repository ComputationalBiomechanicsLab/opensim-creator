#include "OpenSimDecorationOptions.h"

#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Conversion.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Variant/Variant.h>
#include <liboscar/Variant/VariantType.h>

#include <optional>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

osc::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    m_MuscleDecorationStyle{MuscleDecorationStyle::Default},
    m_MuscleColorSource{MuscleColorSource::Default},
    m_MuscleSizingStyle{MuscleSizingStyle::Default},
    m_MuscleColourSourceScaling{MuscleColorSourceScaling::Default},
    m_Flags{OpenSimDecorationOptionFlag::Default}
{}

MuscleDecorationStyle osc::OpenSimDecorationOptions::getMuscleDecorationStyle() const
{
    return m_MuscleDecorationStyle;
}

void osc::OpenSimDecorationOptions::setMuscleDecorationStyle(MuscleDecorationStyle s)
{
    m_MuscleDecorationStyle = s;
}

MuscleColorSource osc::OpenSimDecorationOptions::getMuscleColorSource() const
{
    return m_MuscleColorSource;
}

void osc::OpenSimDecorationOptions::setMuscleColorSource(MuscleColorSource s)
{
    m_MuscleColorSource = s;
}

MuscleSizingStyle osc::OpenSimDecorationOptions::getMuscleSizingStyle() const
{
    return m_MuscleSizingStyle;
}

void osc::OpenSimDecorationOptions::setMuscleSizingStyle(MuscleSizingStyle s)
{
    m_MuscleSizingStyle = s;
}

MuscleColorSourceScaling osc::OpenSimDecorationOptions::getMuscleColorSourceScaling() const
{
    return m_MuscleColourSourceScaling;
}

void osc::OpenSimDecorationOptions::setMuscleColorSourceScaling(MuscleColorSourceScaling s)
{
    m_MuscleColourSourceScaling = s;
}

size_t osc::OpenSimDecorationOptions::getNumOptions() const
{
    return num_flags<OpenSimDecorationOptionFlag>();
}

bool osc::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags.get(GetIthOption(i));
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
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowScapulo);
}

void osc::OpenSimDecorationOptions::setShouldShowScapulo(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowScapulo, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForOrigin() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin);
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForOrigin(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForInsertion() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion);
}

void osc::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForInsertion(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForOrigin() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin);
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForInsertion() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion);
}

void osc::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowCentersOfMass() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass);
}

void osc::OpenSimDecorationOptions::setShouldShowCentersOfMass(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowPointToPointSprings() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings);
}

void osc::OpenSimDecorationOptions::setShouldShowPointToPointSprings(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowContactForces() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowContactForces);
}

void osc::OpenSimDecorationOptions::setShouldShowContactForces(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowContactForces, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowForceLinearComponent() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent);
}

void osc::OpenSimDecorationOptions::setShouldShowForceLinearComponent(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowForceAngularComponent() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent);
}

void osc::OpenSimDecorationOptions::setShouldShowForceAngularComponent(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent, v);
}

bool osc::OpenSimDecorationOptions::getShouldShowPointForces() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowPointForces);
}

void osc::OpenSimDecorationOptions::setShouldShowPointForces(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowPointForces, v);
}

void osc::OpenSimDecorationOptions::setShouldShowEverything(bool v)
{
    setShouldShowScapulo(v);
    setShouldShowEffectiveMuscleLineOfActionForOrigin(v);
    setShouldShowEffectiveMuscleLineOfActionForInsertion(v);
    setShouldShowAnatomicalMuscleLineOfActionForOrigin(v);
    setShouldShowAnatomicalMuscleLineOfActionForInsertion(v);
    setShouldShowCentersOfMass(v);
    setShouldShowPointToPointSprings(v);
    setShouldShowContactForces(v);
    setShouldShowForceLinearComponent(v);
    setShouldShowForceAngularComponent(v);
    setShouldShowPointForces(v);
}

void osc::OpenSimDecorationOptions::forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const Variant&)>& callback) const
{
    callback("muscle_decoration_style", GetMuscleDecorationStyleMetadata(m_MuscleDecorationStyle).id);
    callback("muscle_coloring_style", GetMuscleColoringStyleMetadata(m_MuscleColorSource).id);
    callback("muscle_sizing_style", GetMuscleSizingStyleMetadata(m_MuscleSizingStyle).id);
    callback("muscle_color_scaling", GetMuscleColorSourceScalingMetadata(m_MuscleColourSourceScaling).id);
    for (size_t i = 0; i < num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& meta = GetIthOptionMetadata(i);
        callback(meta.id, Variant{m_Flags.get(GetIthOption(i))});
    }
}

void osc::OpenSimDecorationOptions::tryUpdFromValues(
    std::string_view prefix,
    const std::unordered_map<std::string, Variant>& lut)
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

    if (auto* appVal = lookup("muscle_decoration_style"); appVal and appVal->type() == VariantType::String)
    {
        const auto metadata = GetAllMuscleDecorationStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleDecorationStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_coloring_style"); appVal and appVal->type() == VariantType::String)
    {
        const auto metadata = GetAllPossibleMuscleColoringSourcesMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleColorSource = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_sizing_style"); appVal and appVal->type() == VariantType::String)
    {
        const auto metadata = GetAllMuscleSizingStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleSizingStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_color_scaling"); appVal and appVal->type() == VariantType::String)
    {
        const auto metadata = GetAllPossibleMuscleColorSourceScalingMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleColourSourceScaling = it->value;
        }
    }

    for (size_t i = 0; i < num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& metadata = GetIthOptionMetadata(i);
        if (auto* appVal = lookup(metadata.id); appVal and appVal->type() == VariantType::Bool) {
            m_Flags.set(GetIthOption(i), to<bool>(*appVal));
        }
    }
}
