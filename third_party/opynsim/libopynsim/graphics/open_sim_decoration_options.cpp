#include "open_sim_decoration_options.h"

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

#include <optional>
#include <ranges>

using namespace opyn;
namespace rgs = std::ranges;

opyn::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    m_MuscleDecorationStyle{osc::MuscleDecorationStyle::Default},
    m_MuscleColorSource{osc::MuscleColorSource::Default},
    m_MuscleSizingStyle{osc::MuscleSizingStyle::Default},
    m_MuscleColourSourceScaling{osc::MuscleColorSourceScaling::Default},
    m_Flags{OpenSimDecorationOptionFlag::Default}
{}

osc::MuscleDecorationStyle opyn::OpenSimDecorationOptions::getMuscleDecorationStyle() const
{
    return m_MuscleDecorationStyle;
}

void opyn::OpenSimDecorationOptions::setMuscleDecorationStyle(osc::MuscleDecorationStyle s)
{
    m_MuscleDecorationStyle = s;
}

osc::MuscleColorSource opyn::OpenSimDecorationOptions::getMuscleColorSource() const
{
    return m_MuscleColorSource;
}

void opyn::OpenSimDecorationOptions::setMuscleColorSource(osc::MuscleColorSource s)
{
    m_MuscleColorSource = s;
}

osc::MuscleSizingStyle opyn::OpenSimDecorationOptions::getMuscleSizingStyle() const
{
    return m_MuscleSizingStyle;
}

void opyn::OpenSimDecorationOptions::setMuscleSizingStyle(osc::MuscleSizingStyle s)
{
    m_MuscleSizingStyle = s;
}

osc::MuscleColorSourceScaling opyn::OpenSimDecorationOptions::getMuscleColorSourceScaling() const
{
    return m_MuscleColourSourceScaling;
}

void opyn::OpenSimDecorationOptions::setMuscleColorSourceScaling(osc::MuscleColorSourceScaling s)
{
    m_MuscleColourSourceScaling = s;
}

size_t opyn::OpenSimDecorationOptions::getNumOptions() const
{
    return osc::num_flags<OpenSimDecorationOptionFlag>();
}

bool opyn::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return m_Flags.get(GetIthOption(i));
}

void opyn::OpenSimDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetIthOption(m_Flags, i, v);
}

osc::CStringView opyn::OpenSimDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).label;
}

std::optional<osc::CStringView> opyn::OpenSimDecorationOptions::getOptionDescription(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).maybeDescription;
}

bool opyn::OpenSimDecorationOptions::getShouldShowScapulo() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowScapulo);
}

void opyn::OpenSimDecorationOptions::setShouldShowScapulo(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowScapulo, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForOrigin() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForOrigin(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForInsertion() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForInsertion(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForOrigin() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForInsertion() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowCentersOfMass() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass);
}

void opyn::OpenSimDecorationOptions::setShouldShowCentersOfMass(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowPointToPointSprings() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings);
}

void opyn::OpenSimDecorationOptions::setShouldShowPointToPointSprings(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowContactForces() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowContactForces);
}

void opyn::OpenSimDecorationOptions::setShouldShowContactForces(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowContactForces, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowForceLinearComponent() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent);
}

void opyn::OpenSimDecorationOptions::setShouldShowForceLinearComponent(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowForceAngularComponent() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent);
}

void opyn::OpenSimDecorationOptions::setShouldShowForceAngularComponent(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowPointForces() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowPointForces);
}

void opyn::OpenSimDecorationOptions::setShouldShowPointForces(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowPointForces, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowScholz2015ObstacleContactHints() const
{
    return m_Flags.get(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints);
}

void opyn::OpenSimDecorationOptions::setShouldShowScholz2015ObstacleContactHints(bool v)
{
    m_Flags.set(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints, v);
}

void opyn::OpenSimDecorationOptions::setShouldShowEverything(bool v)
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

void opyn::OpenSimDecorationOptions::forEachOptionAsAppSettingValue(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    callback("muscle_decoration_style", osc::Variant{GetMuscleDecorationStyleMetadata(m_MuscleDecorationStyle).id});
    callback("muscle_coloring_style",   osc::Variant{GetMuscleColoringStyleMetadata(m_MuscleColorSource).id});
    callback("muscle_sizing_style",     osc::Variant{GetMuscleSizingStyleMetadata(m_MuscleSizingStyle).id});
    callback("muscle_color_scaling",    osc::Variant{GetMuscleColorSourceScalingMetadata(m_MuscleColourSourceScaling).id});
    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& meta = GetIthOptionMetadata(i);
        callback(meta.id, osc::Variant{m_Flags.get(GetIthOption(i))});
    }
}

void opyn::OpenSimDecorationOptions::tryUpdFromValues(
    std::string_view prefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
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

    if (auto* appVal = lookup("muscle_decoration_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = osc::GetAllMuscleDecorationStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleDecorationStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_coloring_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = osc::GetAllPossibleMuscleColoringSourcesMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleColorSource = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_sizing_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = osc::GetAllMuscleSizingStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleSizingStyle = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_color_scaling"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = osc::GetAllPossibleMuscleColorSourceScalingMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            m_MuscleColourSourceScaling = it->value;
        }
    }

    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& metadata = GetIthOptionMetadata(i);
        if (auto* appVal = lookup(metadata.id); appVal and appVal->type() == osc::VariantType::Bool) {
            m_Flags.set(GetIthOption(i), to<bool>(*appVal));
        }
    }
}
